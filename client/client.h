#ifndef CLIENT_H
#define CLIENT_H

#include "../typedefs.h"

// Functions:

//Return 1 for IPv4 and 0 for all the others (We expect IPv6 then)
int isIpv4(char *);

//Parse list from from given buffer, that contains serverlist. 
//Present that list to user and ask which server they want to connect.
//Copy server choosed by user to the start of argument buffer
int serverListParser(char *);

//Parse ip and port from character strings to a struct sockaddr_in.
struct sockaddr_in ipv4_parser(char *, char *);

//Parse ip and port from character strings to a struct sockaddr_in6.
struct sockaddr_in6 ipv6_parser(char *, char *);

// Get character from terminal

char getInput(char *);

// Handle received character
char *processCommand(char, char, char *);

#endif
