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
int checkCollision(Gamestate*, Coord);

// Sends announcement to all players except ID
int sendAnnounce(Gamestate*, char*, size_t, ID);

// Register map server to matchmaking server
int registerToMM(char *, char *, char, struct sockaddr_in*);

#endif
