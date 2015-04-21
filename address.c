#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>

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
struct sockaddr_storage ip_parser(char *ip, char *port) {
    struct sockaddr_in6 temp6;
    struct sockaddr_in temp4; 
    struct sockaddr_storage ret_addr;
    memset(&temp4, 0, sizeof(struct sockaddr_in));
    memset(&temp6, 0, sizeof(struct sockaddr_in6));
    memset(&ret_addr, 0, sizeof(struct sockaddr_storage));
    if(isIpv4(ip)) {
	if(inet_pton(AF_INET, ip, &temp4.sin_addr) < 1) {
	    perror("inet_pton");
	}
	temp4.sin_port = ntohs(strtol(port, NULL, 10));
	temp4.sin_family = AF_INET;
	memcpy(&ret_addr, &temp4, sizeof(struct sockaddr_in));
    }
    else {
	if(inet_pton(AF_INET6, ip, &temp6.sin6_addr) < 1) {
	    perror("inet_pton");
	}
	temp6.sin6_port = ntohs(strtol(port, NULL, 10));
	temp6.sin6_family = AF_INET6;
	memcpy(&ret_addr, &temp6, sizeof(struct sockaddr_in6));
    }
    return ret_addr;
}
