/*
Begin-Doc
Name: tcp.h
Description: tcp routine headers/prototypes
End-Doc
*/

#ifndef TCP_H
#define TCP_H

/* Protoypes for functions and global variables in tcp.c */
int readline(char *ptr, int maxlen, int fd);
int writeline(int fd, char *ptr);
int writen(register int fd, register char *ptr, register int nbytes);
char *resolve(char *addr);
char *rresolve(char *addr);
int OpenConnection(char *host, int port);
void CloseConnection(int sockfd);
int OpenListener(int port, int count);
char *clientaddr(int fd);
int clientport(int fd);
int validaddr(char *addr);

#endif
