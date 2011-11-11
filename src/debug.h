/* Debugging Routines */
#ifndef DEBUG_H
#define DEBUG_H

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

#include <stdio.h>

#define Debug(a) if (DEBUG) { printf("%s[%d]: ", ARGV0, getpid()); \
	printf a; fflush(stdout); }
#define Error(a) printf("%s[%d]: ", ARGV0, getpid()); \
	printf a; fflush(stdout);
#define Trace(a) printf("%s[%d]: ", ARGV0, getpid()); \
	printf a; fflush(stdout);
#define NullCk(s)    ((s) ? (s) : "")
extern int DEBUG;
extern char *ARGV0;

#define SFree(x) if (x) { free(x); } \
	else \
	{ Error3( "SFree Failed on '%d' of '%s'\n",__LINE__,__FILE__ ); }

#endif
