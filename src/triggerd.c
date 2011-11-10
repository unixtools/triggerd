#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "debug.h"
#include "tcp.h"

#define TSTAMP_FILE "/local/tsmapp/web/last_publish.txt"

char *exec_cmd = NULL;
char *watch_file = NULL;
int listen_port = 0;
int dry_run = 0;
int daemonize = 1;

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

void DoBuild(void)
{
	printf("Building at: ");
	PrintTime();
	printf("\n\n");
	fflush(stdout);

	system("su tsmapp -c /local/tsmapp/publish/sync-web.pl");

	printf("Done building at: ");
	PrintTime();
	printf("\n\n");
	fflush(stdout);
}

void GetOptions(int argc, char *argv[])
{
    int c;
    int errcnt = 0;

    /* add bind ip */

    while ( (c=getopt(argc, argv, ":de:fnhp:w:") ) != -1 )
    {
        switch(c)
        {
        case 'd':
            DEBUG=1;
            Debug(("cmd line - debugging on\n"));
            break;

        case 'f':
            Debug(("setting server to not daemonize.\n"));
            daemonize = 0;
            break;

        case 'n':
            Debug(("setting server to not actually execute script.\n"));
            dry_run = 1;
            break;

        case 'e':
            Debug(("setting execution cmd to '%s'.\n", optarg));
            exec_cmd = strdup(optarg);
            break;

        case 'p':
            Debug(("setting listen port to '%d'.\n", atoi(optarg)));
            listen_port = atoi(optarg);
            break;

        case 'w':
            Debug(("setting watch trigger file to '%d'.\n", optarg));
            watch_file = strdup(optarg);
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
            Error(("\t -w path  - Enable file/dir watch and sets path\n"));
            Error(("\t -p port  - Enable tcp listen and sets port\n"));
            Error(("\t -e cmd   - Sets execution command\n"));
            exit(1);
        }
    }

}


int main(int argc, char *argv[])
{
	struct stat curstat, oldstat;
	int lasttime;
	int res;

    GetOptions(argc, argv);

	/* Become daemon */
	if ( fork() )
	{
		exit(0);
	}

	/* Change to netdb directory */
	chdir("/local/trigger");

	/* Make sure tstamp file exists */
	close(creat(TSTAMP_FILE, S_IRUSR | S_IRGRP | S_IROTH |
		S_IWUSR | S_IWGRP | S_IWOTH));

	/* Main loop */
	bzero((char *)&oldstat, sizeof(struct stat));
	while ( 1 )
	{	
		res = stat(TSTAMP_FILE, &curstat);
		if ( res )
		{
			printf("Failed to access timestamp file.\n");	
			fflush(stdout);
		}
		else
		{
			if ( curstat.st_mtime != oldstat.st_mtime )
			{
				DoBuild();
				oldstat = curstat;
			}
		}
		sleep(1);
	}
}

