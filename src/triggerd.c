#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include "debug.h"
#include "tcp.h"
#include "util.h"

#define TSTAMP_FILE "/local/tsmapp/web/last_publish.txt"

char *exec_cmd = NULL;

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
    char *file = (char*)threadarg;

	/* Main loop */
	bzero((char *)&oldstat, sizeof(struct stat));
	while ( 1 )
	{	
		res = stat(file, &curstat);
		if ( ! res )
		{
			if ( curstat.st_mtime != oldstat.st_mtime )
			{
				oldstat = curstat;

                if ( skip_first && first )
                {
                    Debug(("mtime of '%s' changed, skipping initial update\n", file));
                }
                else
                {
                    Debug(("mtime of '%s' changed, incrementing updates\n", file));
                    mark_updated();
                }
			}
		}
        else
        {
            Debug(("stat of '%s' failed\n", file));
        }

        first = 0;
		sleep(1);
	}
}

void *thr_watch_port(void *threadarg)
{
   
    while (1)
    {


    }
}



int main(int argc, char *argv[])
{
    pthread_t tids[100];
    int curtid = 0;

    long curupdates = 0;
    int rc;

    int c;
    int errcnt = 0;
    int changed;
    int watches = 0;

    ARGV0 = argv[0];

    /* initialize mutex */
    pthread_mutex_init(&updates_mutex, NULL);

    /* add bind ip */

    while ( (c=getopt(argc, argv, ":de:fnhp:w:s") ) != -1 )
    {
        switch(c)
        {
        case 'd':
            DEBUG=1;
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
            Debug(("setting execution cmd to '%s'.\n", optarg));
            exec_cmd = strdup(optarg);
            break;

        case 'l':
            Debug(("spawning port listen thread for '%s'\n", optarg));
            rc = pthread_create(&tids[curtid++], NULL, thr_watch_port, (void *)strdup(optarg));
            watches++;
            break;

        case 'w':
            Debug(("spawning watch trigger thread for '%s'.\n", optarg));
            rc = pthread_create(&tids[curtid++], NULL, thr_watch_file, (void *)strdup(optarg));
            watches++;
            break;

        case ':':
            fprintf(stderr, "Option -%c requires argument!\n",
                optopt);
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

        if (errcnt)
        {
            Error(("Usage:\n"));
            Error(("\t%s <arguments>\n\n", ARGV0));
            Error(("\t -h       - Prints help\n"));
            Error(("\t -f       - Stay in foreground\n"));
            Error(("\t -d       - Enables debugging output\n"));
            Error(("\t -w path  - Enable file/dir watch and sets path (can be repeated)\n"));
            Error(("\t -p port  - Enable tcp listen and sets port (can be repeated)\n"));
            Error(("\t -e cmd   - Sets execution command\n"));
            Error(("\t -s       - Skip first update at startup\n"));
            exit(1);
        }
    }

	/* Become daemon */
    if ( daemonize )
    {
        BeDaemon(argv[0]);
    }

    if ( ! exec_cmd )
    {
        Error(("No exec cmd specified, terminating.\n"));
    }

    if ( ! watches )
    {
        Error(("No watch conditions specified, terminating.\n"));
    }

    /* main loop running updates */
    while ( 1 )
    {
        changed = 0;

        pthread_mutex_lock(&updates_mutex);
        if ( curupdates != updates )
        {
            curupdates = updates;
            changed = 1;
        }
        pthread_mutex_unlock(&updates_mutex);
        
        if ( changed )
        {
            Debug(("update triggered, executing commands...\n"));

/* should do with popen so we can support syslog or stdout within daemon */
            system(exec_cmd);
        }
        sleep(1);
    }
}

