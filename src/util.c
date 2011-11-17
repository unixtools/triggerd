/*
Begin-Doc
Name: util.c
Description: utility/helper routines
End-Doc
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "debug.h"

/* 
Begin-Doc
Name: BeDaemon
Description: daemonize process
Syntax: BeDaemon(progname)
Returns: process id
End-Doc
*/
int BeDaemon(char *pgm)
{
    int childpid;
    int curpid;

    Debug(("%s: %s\n", pgm, "forking into background"));
    if ((childpid = fork()) < 0) {
        Error(("%s: %s\n", pgm, "initial fork error"));
        exit(1);
    } else if (childpid != 0) {
        exit(0);
    }

    curpid = getpid();
    Debug(("%s: %s (%d)\n", pgm, "background process id", curpid));

    return curpid;
}
