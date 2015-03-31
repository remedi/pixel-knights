#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <netinet/in.h>

#include "../typedefs.h"

// Returns the maximum of a and b
int max(int, int);

// Create new ID for new client connection
ID createID(void);

// Check that the coordinate is valid 
int checkCoordinate(int);

// Sends announcement to all players except ID
int sendAnnounce(Gamestate*, char*, size_t, ID);

// Announce map server to matchmaking server
int connectMM(char *, char *, char, struct sockaddr_in*);

#endif
