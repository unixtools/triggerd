/*
Begin-Doc
Name: triggerd.d
Description: Main program for triggerd code
End-Doc
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <error.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <pwd.h>
#include <fcntl.h>

#include "debug.h"
#include "tcp.h"
#include "util.h"

int dry_run = 0;
int daemonize = 1;
int skip_first = 0;

long updates = 0;
pthread_mutex_t updates_mutex;

/* 
Begin-Doc
Name: mark_updated
Description: log and set globals to indicate trigger has fired
Syntax: mark_updated();
End-Doc
*/
void mark_updated(void)
{
    pthread_mutex_lock(&updates_mutex);
    Debug(("updates current value (%ld)\n", updates));
    updates++;
    Debug(("updates new value (%ld)\n", updates));
    pthread_mutex_unlock(&updates_mutex);
}

/* 
Begin-Doc
Name: thr_watch_file
Description: thread handler to watch a file
Syntax: thr_watch_file(filename)
End-Doc
*/
void *thr_watch_file(void *threadarg)
{
    struct stat curstat, oldstat;
    int res;
    int first = 1;
    char *file = (char *)threadarg;

    Debug(("starting file watch of '%s'\n", file));

    /* Main loop */
    bzero((char *)&oldstat, sizeof(struct stat));
    while (1) {
        res = stat(file, &curstat);
        if (!res) {
            if (curstat.st_mtime != oldstat.st_mtime) {
                oldstat = curstat;

                if (skip_first && first) {
                    Debug(("mtime of '%s' changed, skipping initial update\n",
                           file));
                } else {
                    Debug(("mtime of '%s' changed, incrementing updates\n",
                           file));
                    mark_updated();
                    syslog(LOG_DEBUG,
                           "mtime of '%s' changed, triggering updates", file);
                }
            }
        } else {
            Debug(("stat of '%s' failed\n", file));
        }

        first = 0;
        sleep(1);
    }
}

/* 
Begin-Doc
Name: thr_watch_port
Description: thread handler to listen to a tcp port
Syntax: thr_watch_port(port_in_string_form)
End-Doc
*/
void *thr_watch_port(void *threadarg)
{
    int sockfd;
    int server_port;
    struct sockaddr_in cli_addr;
    unsigned int clilen;
    int newsockfd;
    char *cliaddr;

    server_port = atoi((char *)threadarg);

    Debug(("opening listener socket on port %d\n", server_port));
    syslog(LOG_DEBUG, "opening listener socket on port %d", server_port);
    sockfd = OpenListener(server_port, 10);
    if (!sockfd) {
        Error(("Unable to open listener port! (%s)\n", strerror(errno)));
        syslog(LOG_ERR, "unable to open listener port (%s)\n", strerror(errno));
        exit(1);
    }
    Debug(("opened listener socket on port %d\n", server_port));
    syslog(LOG_DEBUG, "opened listener socket on port %d", server_port);

    fcntl(sockfd, F_SETFD, fcntl(sockfd, F_GETFD) | FD_CLOEXEC);

    while (1) {
        clilen = sizeof(cli_addr);

        Debug(("waiting for socket connection\n"));
        syslog(LOG_DEBUG, "waiting for socket connection");

        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (newsockfd) {
            syslog(LOG_DEBUG, "got a socket connection");
            Debug(("got a socket connection\n"));
            cliaddr = clientaddr(newsockfd);

            syslog(LOG_DEBUG,
                   "got a socket connection on port %d from %s, triggering updates",
                   server_port, NullCk(cliaddr));

            if (cliaddr) {
                free(cliaddr);
            }

            mark_updated();
            close(newsockfd);
        } else {
            syslog(LOG_ERR,
                   "error accepting socket connection (%s)", strerror(errno));
        }
    }
}

#define MAX_CMDS 100
#define MAX_THREADS 100

/* 
Begin-Doc
Name: main
Description: main program
End-Doc
*/
int main(int argc, char *argv[])
{
    pthread_t tids[MAX_THREADS];
    int curtid = 0;

    char *cmds[MAX_CMDS + 1];
    int numcmds = 0;;

    char *files[MAX_CMDS + 1];
    int numfiles = 0;

    char *ports[MAX_CMDS + 1];
    int numports = 0;

    long curupdates = 0;
    int rc;

    int c;
    int i;
    int errcnt = 0;
    int changed;
    int watches = 0;

    char *setuser = NULL;

    ARGV0 = argv[0];

    /* Init syslogs */
    openlog(NULL, LOG_PID, LOG_DAEMON);

    /* initialize mutex */
    pthread_mutex_init(&updates_mutex, NULL);

    /* add bind ip */

    while ((c = getopt(argc, argv, ":dfsne:p:w:ht:u:")) != -1) {
        switch (c) {
        case 'd':
            DEBUG = 1;
            Debug(("enabled debug output\n"));
            break;

        case 'f':
            Debug(("setting server to not daemonize.\n"));
            daemonize = 0;
            break;

        case 's':
            Debug(("skipping first update.\n"));
            skip_first = 1;
            break;

        case 'n':
            Debug(("setting server to not actually execute script.\n"));
            dry_run = 1;
            break;

        case 'e':
            if (numcmds >= MAX_CMDS) {
                Error(("Max cmds reached!\n"));
                exit(1);
            } else {
                Debug(("adding execution cmd '%s'.\n", optarg));
                cmds[numcmds++] = strdup(optarg);
            }
            break;

        case 'p':
            if (numports >= MAX_CMDS) {
                Error(("Max ports reached!\n"));
                exit(1);
            } else {
                Debug(("adding port listen '%s'.\n", optarg));
                ports[numports++] = strdup(optarg);
                watches++;
            }
            break;

        case 'w':
            if (numfiles >= MAX_CMDS) {
                Error(("Max file watches reached!\n"));
                exit(1);
            } else {
                Debug(("adding file watch '%s'.\n", optarg));
                files[numfiles++] = strdup(optarg);
                watches++;
            }
            break;

        case 't':
            Debug(("setting syslog tag '%s'.\n", optarg));
            openlog(optarg, LOG_PID, LOG_DAEMON);
            break;

        case 'u':
            Debug(("will run as user '%s'.\n", optarg));
            setuser = strdup(optarg);
            break;

        case ':':
            fprintf(stderr, "Option -%c requires argument!\n", optopt);
            errcnt++;
            break;

        case '?':
            fprintf(stderr, "Unknown option -%c\n", optopt);
            errcnt++;
            break;

        case 'h':
            errcnt++;
            break;
        }

        if (errcnt) {
            Error(("Usage:\n"));
            Error(("\t%s <arguments>\n\n", ARGV0));
            Error(("\t -h        - Prints help\n"));
            Error(("\t -f        - Stay in foreground\n"));
            Error(("\t -d        - Enables debugging output\n"));
            Error(("\t -w path   - Enable file/dir watch and sets path (can be repeated)\n"));
            Error(("\t -p port   - Enable tcp listen and sets port (can be repeated)\n"));
            Error(("\t -e cmd    - Sets execution command (can be repeated)\n"));
            Error(("\t -s        - Skip first update at startup\n"));
            Error(("\t -t tag    - Enable syslog output labelled with 'tag'\n"));
            Error(("\t -u userid - Set uid/gid to user 'userid'\n"));
            exit(1);
        }
    }

    if (numcmds == 0) {
        Error(("No exec cmd specified, terminating.\n"));
        exit(1);
    }

    if (watches == 0) {
        Error(("No watch conditions specified, terminating.\n"));
        exit(1);
    }

    if (setuser) {
        struct passwd *user;

        if (!(user = getpwnam(setuser))) {
            Error(("Failed to look up user '%s'\n", setuser));
            exit(1);
        }

        if (setgid(user->pw_gid)) {
            Error(("Failed to switch gid to '%d' for user '%s'\n",
                   user->pw_gid, setuser));
            exit(1);
        }
        if (setuid(user->pw_uid)) {
            Error(("Failed to switch uid to '%d' for user '%s'\n",
                   user->pw_uid, setuser));
            exit(1);
        }

    }

    if (numfiles + numports > MAX_THREADS) {
        Error(("Too many commands/files, won't have enough thread slots!\n"));
        exit(1);
    }

    /* Become daemon */
    if (daemonize) {
        syslog(LOG_INFO, "daemonizing");
        BeDaemon(argv[0]);
    }

    for (i = 0; i < numcmds; i++) {
        syslog(LOG_INFO, "configured execution cmd: %s", cmds[i]);
    };
    for (i = 0; i < numfiles; i++) {
        Debug(("spawning watch trigger thread for '%s'.\n", files[i]));
        syslog(LOG_INFO, "spawning watch trigger thread for '%s'", files[i]);

        rc = pthread_create(&tids[curtid++], NULL, thr_watch_file,
                            (void *)files[i]);
        if (rc) {
            syslog(LOG_ERR, "thread spawn failed: %d", rc);
            exit(1);
        }
    };

    for (i = 0; i < numports; i++) {
        Debug(("spawning port listen thread for '%s'\n", ports[i]));
        syslog(LOG_INFO, "spawning port listen thread for '%s'", ports[i]);

        rc = pthread_create(&tids[curtid++], NULL, thr_watch_port,
                            (void *)ports[i]);
        if (rc) {
            syslog(LOG_ERR, "thread spawn failed: %d", rc);
            exit(1);
        }
    }

    syslog(LOG_INFO, "starting main processing loop");
    /* main loop running updates */
    while (1) {
        changed = 0;

        pthread_mutex_lock(&updates_mutex);
        if (curupdates != updates) {
            curupdates = updates;
            changed = 1;
        }
        pthread_mutex_unlock(&updates_mutex);

        if (changed) {
            Debug(("update triggered, executing commands...\n"));
            syslog(LOG_INFO, "update triggered, executing commands");

/* should do with popen so we can support syslog or stdout within daemon */
            for (i = 0; i < numcmds; i++) {
                FILE *cmdfh;
                char *tmpcmd;

                Debug(("Executing: %s\n", cmds[i]));
                syslog(LOG_INFO, "executing: %s", cmds[i]);

                tmpcmd = calloc(strlen(cmds[i]) + 10, 1);
                if (!tmpcmd) {
                    Debug(("Failed to allocate memory for command!\n"));
                    syslog(LOG_ERR, "failed to allocate memory for command");
                } else {
                    snprintf(tmpcmd, strlen(cmds[i]) + 10, "%s 2>&1", cmds[i]);
                    cmdfh = popen(tmpcmd, "r");
                    free(tmpcmd);
                    if (cmdfh) {
                        char cmdbuf[5000];

                        while (fgets(cmdbuf, 5000, cmdfh)) {
                            if (!daemonize) {
                                Trace(("%s", cmdbuf));
                            }
                            syslog(LOG_DEBUG, "%s", cmdbuf);
                        }
                        fclose(cmdfh);
                    } else {
                        Debug(("Failed to open pipe for command!\n"));
                        syslog(LOG_ERR, "failed to open pipe for command");
                    }
                }
            }

            Debug(("execution of commands completed...\n"));
            syslog(LOG_INFO, "execution of commands completed");
        }
        sleep(1);
    }
}
