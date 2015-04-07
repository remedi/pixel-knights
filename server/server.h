#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <netinet/in.h>

#include "../typedefs.h"

// Processes the Action received from the client
int processAction(Gamestate*, Mapdata *, ID, Action);

//Move each bullet once
int updateBullets(Gamestate*, Mapdata*);

// Spawn tree to a random location
int spawnScorePoint(Gamestate*, Mapdata*);

//Generate random coordinates, that are not occupied by anything from the map
int randomCoord(Gamestate *, Mapdata *, Coord *);

// Returns the maximum of a and b
int max(int, int);

// Create new ID for new client connection
ID createID(Gamestate*);

// Check that the coordinate is valid 
ID checkCollision(Gamestate*, Coord);

// Sends announcement to all players except ID
int sendAnnounce(Gamestate*, char*, size_t, ID);

// Register map server to matchmaking server
int registerToMM(char *, char *, char, struct sockaddr_in*, struct sockaddr_in6*);

#endif
