#ifndef ADDRESS_H
#define ADDRESS_H


#include "typedefs.h"

//Return 1 for IPv4 and 0 for all the others (We expect IPv6 then)
int isIpv4(char *);

//Parse ip and port from character strings to a struct sockaddr_in.
struct sockaddr_storage ipv4_parser(char *, char *);

//Parse ip and port from character strings to a struct sockaddr_in6.
struct sockaddr_storage ip_parser(char *, char *);

#endif
