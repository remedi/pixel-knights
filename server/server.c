// This file defines the functions declared in server.h
//
// Authors: Pixel Knights
// Date: 6.3.2015

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "../maps/maps.h"
#include "../address.h"
#include "server.h"
#include "gamestate.h"

// Announce map server to matchmaking server
int registerToMM(char *MM_IP, char *MM_port, char map_nr, struct sockaddr_in* my_IP, struct sockaddr_in6* my_IP6) {
    int sock;
    struct sockaddr_in sock_addr_in;
    struct sockaddr_in6 sock_addr_in6;
    char message[2];
    socklen_t my_IP_len = sizeof(struct sockaddr_in);
    socklen_t my_IP6_len = sizeof(struct sockaddr_in6);
    int IP4;

    // The message sent to MM server contains 'S' + map number
    message[0] = 'S';
    message[1] = map_nr;

    // Parse address and port
    IP4 = isIpv4(MM_IP);
    if(IP4) {
	sock_addr_in = ipv4_parser(MM_IP, MM_port);
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	    perror("MM socket, create");
	    return -1;
	}
	if (connect(sock, (struct sockaddr *) &sock_addr_in, sizeof(sock_addr_in)) == -1) {
	    perror("MM socket, connect");
	    return -1;
	}
    }
    else {
	sock_addr_in6 = ipv6_parser(MM_IP, MM_port);
	if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
	    perror("MM socket, create");
	    return -1;
	}
	if (connect(sock, (struct sockaddr *) &sock_addr_in6, sizeof(sock_addr_in6)) == -1) {
	    perror("MM socket, connect");
	    return -1;
	}
    }

    // Send server information
    if (write(sock, message, 2) == -1) {
        perror("MM socket, write");
        return -1;
    }
    // Wait for ok reply
    if (read(sock, message, 1) == -1) {
        perror("MM socket, read");
        return -1;
    }

    // OK reply from MM
    if (message[0] == 'O') {
	if(IP4) {
	    if (getsockname(sock, (struct sockaddr *) my_IP, &my_IP_len) == -1) {
		perror("getsockname");
		return -1;
	    } 
	}
	else {
	    if (getsockname(sock, (struct sockaddr *) my_IP6, &my_IP6_len) == -1) {
		perror("getsockname");
		return -1;
	    } 
	}
        close(sock);
        return 0;
    }
    // Something went wrong
    else {
        close(sock);
        return -2;
    }
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

// Checks for player collisions in target coordinate
int checkCollision(Gamestate* g, Coord c) {

    // If Gamestate NULL
    if (!g)
        return -1;

    // Linked-list is empty
    if (g->next == NULL)
        return -2;

    // First element does not contain player
    g = g->next;

    while(g) {
        if (g->c.x == c.x && g->c.y == c.y) {
            // Collision happened!
            return -3;
        }
        g = g->next;
    }
    // No collision
    return 0;
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

