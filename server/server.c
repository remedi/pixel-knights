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
#include "server.h"

// Announce map server to matchmaking server
int connectMM(char *IP, char *port, char map_nr, struct sockaddr_in* my_IP) {
    int sock;
    struct sockaddr_in sock_addr_in;
    char message[2];
    socklen_t my_IP_len = sizeof(struct sockaddr_in);

    // The message sent to MM server contains 'S' + map number
    message[0] = 'S';
    message[1] = map_nr;

    // Parse address and port
    memset(&sock_addr_in, 0, sizeof(struct sockaddr_in));
    if(inet_pton(AF_INET, IP, &sock_addr_in.sin_addr) < 1) {
        perror("ipv4_parser, inet_pton");
        return -1;
    }
    sock_addr_in.sin_port = htons(strtol(port, NULL, 10));
    sock_addr_in.sin_family = AF_INET;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("MM socket, create");
        return -1;
    }
    if (connect(sock, (struct sockaddr *) &sock_addr_in, sizeof(sock_addr_in)) == -1) {
        perror("MM socket, connect");
        return -1;
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
       if (getsockname(sock, (struct sockaddr *) my_IP, &my_IP_len) == -1) {
            perror("getsockname");
            return -1;
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


