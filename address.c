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

//Parse ip and port from character strings to a struct sockaddr_in6.
struct sockaddr_in6 ipv6_parser(char *ip, char *port) {
  struct sockaddr_in6 temp;
  if(inet_pton(AF_INET6, ip, &temp.sin6_addr) < 1) {
    perror("ipv6_parser, inet_pton");
    exit(-1);
  }
  temp.sin6_port = ntohs(strtol(port, NULL, 10));
  temp.sin6_family = AF_INET6;
  return temp;
}

//Parse ip and port from character strings to a struct sockaddr_in.
struct sockaddr_in ipv4_parser(char *ip, char *port) {
    struct sockaddr_in temp; 
    memset(&temp, 0, sizeof(struct sockaddr_in));
    if(inet_pton(AF_INET, ip, &temp.sin_addr) < 1) {
        perror("ipv4_parser, inet_pton");
    }
    temp.sin_port = ntohs(strtol(port, NULL, 10));
    temp.sin_family = AF_INET;
    return temp;
}
