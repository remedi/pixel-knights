// This file defines the functions declared in server.h
//
// Authors: Pixel Knights
// Date: 6.3.2015

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "../maps/maps.h"
#include "server.h"


int connectMM(char *IP, char *port, char map_nr, struct sockaddr_in *this_IP) {
    int sock;
    struct sockaddr_in sock_addr_in;
    char new_server[2];
    char *buf = malloc(10);
    socklen_t this_IP_len;

    this_IP_len = sizeof(struct sockaddr_in);


    memset(buf, '\0', 10);

    new_server[0] = 'S';
    new_server[1] = map_nr;

    //Parse address and port
    memset(&sock_addr_in, 0, sizeof(struct sockaddr_in));
    if(inet_pton(AF_INET, IP, &sock_addr_in.sin_addr) < 1) {
        perror("ipv4_parser, inet_pton");
    }
    //sock_addr_in.sin_port = ntohs(strtol(port, NULL, 10));
    sock_addr_in.sin_port = htons(strtol(port, NULL, 10));
    sock_addr_in.sin_family = AF_INET;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
	perror("socket");
	return -1;
    }
    printf("Connecting to MM server..\n");
    if(connect(sock, (struct sockaddr *) &sock_addr_in, sizeof(sock_addr_in)) == -1) {
	perror("MM, connect");
	return -1;
    }
    printf("Connection succeeded. Sending server information..\n");
    //Send server information
    if(write(sock, new_server, 2) == -1) {
	perror("write");
	return -1;
    }
    //Wait for ok reply
    if(read(sock, buf, 1) == -1) {
	perror("write");
	return -1;
    }

    if(buf[0] == 'O') {
	printf("Got succesful acknowledgement from mm server\n");
    }
    else {
	printf("Error when communicatin with mm server\n");
	return -1;
    }
    //Get own address
    if(getsockname(sock, (struct sockaddr *) this_IP, &this_IP_len) == -1) {
	perror("getsockname");
	return -1;
    }
    if(close(sock) == -1) {
	perror("MM socket, close");
    }

    return 0;
}

// Returns the maximum of a and b
int max(int a, int b) {

    if (a > b)
        return a;
    return b;
}

// Returns the ID of a new client connection
ID createID(void) {

    static ID id = 0x01;
    return id++;
}

// Check that the coordinate is valid 
// NOTE: THIS IS PRELIMINARY 
int checkCoordinate(int c) {

    // Just perform some checking
    if (c < 1)
        return 0;
    if (c > 8)
        return 0;
    return 1;
}

// Sends announcement to all players
int sendAnnounce(Gamestate* g, char* msg, size_t len, ID id) {

    // If gamestate NULL
    if (!g)
        return -1;

    // Linked-list is empty
    if (g->next == NULL)
        return -2;

    // First element does not contain player
    g = g->next;

    // Send announcement to all players
    while (g != NULL) {
        if (g->id == id) {
            g = g->next;
            continue;
        }
        if (send(g->sock, msg, len, 0) < 0) {
            perror("send error");
            return -3;
        }
        g = g->next;
    }
    return 0;
}


