#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>

#include "address.h"

//Return 1 for IPv4 and 0 for all the others (We expect IPv6 then)
int isIpv4(char *buf) {
    int len = strlen(buf);
    int dot_count = 0, colon_count = 0, i;
    for(i = 0; i < len; i++) {
	if(buf[i] == '.')
	    dot_count++;
	if(buf[i] == ':')
	    colon_count++;
    }
    //Simple sanity check:
    if(dot_count == 0 && colon_count == 0)
	return -1;

    if(dot_count > colon_count)
	return 1;
    else
	return 0;
}

//Parse ip and port from character strings to a type: struct sockaddr_storage
struct addrinfo ip_parser(char *ip, char *port) {
    struct addrinfo *addr;
    struct addrinfo retaddr;
    struct addrinfo hints;
    memset(&retaddr, 0, sizeof(struct addrinfo));
    //memset(*addr, 0, sizeof(struct addrinfo));
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(ip, port, &hints, &addr) != 0) {
	perror("getaddrinfo");
    }
    memcpy(&retaddr, addr, sizeof(struct addrinfo));
    memcpy(&retaddr.ai_addr, addr->ai_addr, sizeof(addr->ai_addrlen));
    //memcpy(&(retaddr.ai_addr), addr->ai_addr, addr->ai_addrlen);
    retaddr.ai_next = NULL;
    freeaddrinfo(addr);
    return retaddr;
}
