// This file defines the functions declared in server.h
//
// Authors: Pixel Knights
// Date: 6.3.2015

#include <sys/socket.h>

#include "server.h"
#include "typedefs.h"

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


