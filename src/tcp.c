/*
File:		tcp.c
Module:		TCP library routines
Description:	Routines for use by all the other modules for doing anything
		related to access to the sockets, transmissions of data, etc.
*/

/*
 Routines for manipulating tcp/socket related structures in the clients
 and servers.
 
 Some TCP routines adapted from those in UNIX Network Programming, 
 W. Richard Stevens 
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "debug.h"
#include "tcp.h"

/*
 * Convert a string to lowercase in place
 */
void stolower(char *str)
{
    int i;
    if (!str) {
        return;
    }

    for (i = 0; i <= strlen(str); i++) {
        str[i] = tolower(str[i]);
    }
}

/*
 * Read a line from a socket, similar to fgets for file i/o.
 */

int readline(char *ptr, int maxlen, int fd)
{
    int n, rc;
    char c;

    for (n = 1; n < maxlen; n++) {
        if ((rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0) {
            if (n == 1)
                return (0);     /* EOF, no data */
            else
                break;          /* EOF, w/ data */
        } else
            return (-1);        /* Error */
    }

    *ptr = 0;
    return (n);
}

/*
 * Write a line from a null terminated string to a socket
 */

int writeline(int fd, char *ptr)
{
    return writen(fd, ptr, strlen(ptr));
}

int writen(register int fd, register char *ptr, register int nbytes)
{
    int nleft, nwritten;

    nleft = nbytes;
    while (nleft > 0) {
        nwritten = write(fd, ptr, nleft);
        if (nwritten <= 0)
            return (nwritten);

        nleft -= nwritten;
        ptr += nwritten;
    }
    return (nbytes - nleft);
}

/*
 * Resolve a host address from either a dotted quad or a FQDN
 */

char *resolve(char *addr)
{
    static char address[40];
    struct hostent *hostptr;
    struct in_addr *ptr;
    char **listptr;

    strcpy(address, addr);
    if (!isdigit(address[0])) {
        hostptr = gethostbyname(addr);
        /* static data */
        if (!hostptr) {
            return ((char *)0);
        }

        listptr = hostptr->h_addr_list;
        ptr = (struct in_addr *)*listptr;
        strcpy(address, inet_ntoa(*ptr));
        return address;
    } else {
        if (!validaddr(address)) {
            return ((char *)0);
        }
        return address;
    }
    return "";
}

/*
 * Resolve a host address from either a dotted quad or a FQDN
 */

char *rresolve(char *addr)
{
    static char address[100];
    struct hostent *hostptr;
    struct in_addr inaddr;

    inaddr.s_addr = inet_addr(addr);
    hostptr = gethostbyaddr((char *)&inaddr, sizeof(inaddr), AF_INET);
    /* static data */

    if (!hostptr) {
        Debug(("gethostbyaddr failed.\n"));
        return ((char *)0);
    }
    Debug(("reverse dns (%s) = (%s)\n", addr, hostptr->h_name));
    strcpy(address, hostptr->h_name);
    stolower(address);
    return address;
}

/*
 * Open an active connection to a host returning the socket file descriptor
 */

int OpenConnection(char *host, int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    char *resolvedhost;
    int bufsize;

    Debug(("attempting to open connection to (%s:%d)\n", host, port));

    /* Bind address */
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    resolvedhost = resolve(host);
    if (resolvedhost == NULL) {
        Debug(("couldn't resolve host (%s)\n", host));
        return 0;
    }
    Debug(("resolved host to (%s)\n", resolvedhost));

    serv_addr.sin_addr.s_addr = inet_addr(resolvedhost);
    serv_addr.sin_port = htons(port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return 0;
    }

    bufsize = 32000;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (void *)&bufsize,
               sizeof(bufsize));

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        return 0;
    }

    return sockfd;
}

/* 
 * Close a connection to a socket
 */

void CloseConnection(int sockfd)
{
    close(sockfd);
}

/*
 * Open a passive listener connection on a port, returning the socket fd
 */

int OpenListener(int port, int count)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    int tmpint;

    Debug(("opening listener port(%d) queue(%d)\n", port, count));

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return 0;
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    tmpint = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&tmpint,
               sizeof(tmpint));

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        return 0;
    }

    tmpint = 32000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (void *)&tmpint, sizeof(tmpint));
    listen(sockfd, count);

    return sockfd;
}

/*
 * Return a string containing the ip address of the host that is connected on 
 * this socket.
 */

char *clientaddr(int fd)
{
    struct sockaddr_in cli_addr;
    static char address[40];
    int len;

    len = sizeof(cli_addr);
    getpeername(fd, (struct sockaddr *)&cli_addr, (void*) &len);

    strcpy(address, inet_ntoa(cli_addr.sin_addr));

    return address;
}

int clientport(int fd)
{
    struct sockaddr_in cli_addr;
    int len;

    len = sizeof(cli_addr);
    getpeername(fd, (struct sockaddr *)&cli_addr, &len);
    return cli_addr.sin_port;
}

/*
 * Check if a address string is actually a valid address (format wise)
 */

int validaddr(char *addr)
{
    char address[40];
    char *a1, *a2, *a3, *a4;
    int i;

    if (!addr) {
        return (1);
    }

    for (i = 0; i < strlen(addr); i++) {
        if (!isdigit(addr[i]) || (addr[i] == '.')) {
            return (1);
        }
    }

    strcpy(address, addr);
    a1 = (char *)strtok(address, ".");
    a2 = (char *)strtok(NULL, ".");
    a3 = (char *)strtok(NULL, ".");
    a4 = (char *)strtok(NULL, ".");

    if (!a1 || !a2 || !a3 || !a4) {
        return (1);
    }

    return (0);
}
