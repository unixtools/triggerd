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

#include "debug.h"
#include "tcp.h"
#include "util.h"

int dry_run = 0;
int daemonize = 1;
int skip_first = 0;

long updates = 0;
pthread_mutex_t updates_mutex;

void PrintTime(void)
{
    char outstr[200];
    time_t t;
    struct tm *tmp;

    outstr[0] = 0;

    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
        perror("localtime");
        return;
    }

    if (strftime(outstr, sizeof(outstr), "%F %T", tmp) == 0) {
        fprintf(stderr, "strftime returned 0");
        return;
    }

    printf("%s", outstr);
}

void mark_updated(void)
{
    pthread_mutex_lock(&updates_mutex);
    Debug(("updates current value (%ld)\n", updates));
    updates++;
    Debug(("updates new value (%ld)\n", updates));
    pthread_mutex_unlock(&updates_mutex);
}

void *thr_watch_file(void *threadarg)
{
    struct stat curstat, oldstat;
    int res;
    int first = 1;
    char *file = (char *)threadarg;

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
                }
            }
        } else {
            Debug(("stat of '%s' failed\n", file));
        }

        first = 0;
        sleep(1);
    }
}

void *thr_watch_port(void *threadarg)
{
    int sockfd;
    int server_port;
    struct sockaddr_in cli_addr;
    unsigned int clilen;
    int newsockfd;

    server_port = atoi((char *)threadarg);

    sockfd = OpenListener(server_port, 2048);
    if (!sockfd) {
        Error(("Unable to open listener port! (%s)\n", strerror(errno)));
    }

    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        Debug(("got a socket connection\n"));

        mark_updated();

        if (newsockfd) {
            close(newsockfd);
        }
    }
}

#define MAX_CMDS = 100;

int main(int argc, char *argv[])
{
    pthread_t tids[100];
    int curtid = 0;
    char *cmds[MAX_CMDS + 1];
    int numcmds = 0;;

    long curupdates = 0;
    int rc;

    int c;
    int i;
    int errcnt = 0;
    int changed;
    int watches = 0;

    ARGV0 = argv[0];

    /* initialize mutex */
    pthread_mutex_init(&updates_mutex, NULL);

    /* add bind ip */

    while ((c = getopt(argc, argv, ":dfsne:p:w:h")) != -1) {
        if (numcmds == MAX_CMDS) {
            Error(("Max cmds reached!\n"));
            exit(1);
        }
        if (curtid == MAX_THREADS) {
            Error(("Max threads reached!\n"));
            exit(1);
        }

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
            Debug(("adding execution cmd '%s'.\n", optarg));
            cmds[numcmds++] = strdup(optarg);
            break;

        case 'p':
            Debug(("spawning port listen thread for '%s'\n", optarg));
            rc = pthread_create(&tids[curtid++], NULL,
                                thr_watch_port, (void *)strdup(optarg));
            watches++;
            break;

        case 'w':
            Debug(("spawning watch trigger thread for '%s'.\n", optarg));
            rc = pthread_create(&tids[curtid++], NULL,
                                thr_watch_file, (void *)strdup(optarg));
            watches++;
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
            Error(("\t -h       - Prints help\n"));
            Error(("\t -f       - Stay in foreground\n"));
            Error(("\t -d       - Enables debugging output\n"));
            Error(("\t -w path  - Enable file/dir watch and sets path (can be repeated)\n"));
            Error(("\t -p port  - Enable tcp listen and sets port (can be repeated)\n"));
            Error(("\t -e cmd   - Sets execution command (can be repeated)\n"));
            Error(("\t -s       - Skip first update at startup\n"));
            exit(1);
        }
    }

    /* Become daemon */
    if (daemonize) {
        BeDaemon(argv[0]);
    }

    if (numcmds == 0) {
        Error(("No exec cmd specified, terminating.\n"));
        exit(1);
    }

    if (watches == 0) {
        Error(("No watch conditions specified, terminating.\n"));
        exit(1);
    }

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

/* should do with popen so we can support syslog or stdout within daemon */
            for (i = 0; i < numcmds; i++) {
                Debug(("Executing: %s\n", cmds[i]));
                system(cmds[i]);
            }

            Debug(("execution of commands completed...\n"));
        }
        sleep(1);
    }
}
