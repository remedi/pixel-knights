#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <inttypes.h>

#include "../maps/maps.h"
#include "../typedefs.h"

/* Functions that add to or modify the list:*/

// Adds player to the gamestate linked-list
int addObject(Gamestate*, ID, Coord, int, char, Type, char*);

// Changes the sign of the player
int changePlayerSign(Gamestate*, ID, char);

// Removes player from the gamestate linked-list
Gamestate* removeObject(Gamestate*, ID);

// Terminates the Gamestate instance
void freeGamestate(Gamestate*);


/* Functions that provide information about the list:*/

// Finds pointer to the Gamestate struct containing ID
Gamestate* findObject(Gamestate*, ID);

// Gets the amount of elements in Gamestate
uint8_t getSize(Gamestate*);

// Gets the amount of players in Gamestate
uint8_t getPlayerCount(Gamestate*);

// Gets the amount of trees in Gamestate
uint8_t getScorePointCount(Gamestate*);

// Produces string that contains all data about players
int printObjects(Gamestate*);

// Parses the data that gets sent to the players
int parseGamestate(Gamestate*, void*, int);

#endif
