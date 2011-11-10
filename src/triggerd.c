#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define TSTAMP_FILE "/local/tsmapp/web/last_publish.txt"

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

int main(int argc, char *argv[])
{
	struct stat curstat, oldstat;
	int lasttime;
	int res;

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


