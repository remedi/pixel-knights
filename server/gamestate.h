#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <inttypes.h>

#include "../maps/maps.h"
#include "../typedefs.h"

/* Functions that add something to the list:*/

// Adds player to the gamestate linked-list
int addObject(Gamestate*, ID, Coord, int, char, Type);

// Add bullet next to a shooting player
int addBullet(Gamestate*, Mapdata *, ID, Action);

// Spawn tree to a random location
int spawnTree(Gamestate*, Mapdata*);



/*Functions that modify the list:*/

// Moves player to the destination Coord
int movePlayer(Gamestate*, Mapdata *, ID, Action);

//Move each bullet once
int updateBullets(Gamestate*, Mapdata*);

// Changes the sign of the player
int changePlayerSign(Gamestate*, ID, char);

// Removes player from the gamestate linked-list
int removePlayer(Gamestate*, ID);

// Terminates the Gamestate instance
void freePlayers(Gamestate*);



/* Functions that provide information about the list:*/

// Finds pointer to the Gamestate struct containing ID
Gamestate* findPlayer(Gamestate*, ID);

// Gets the amount of elements in Gamestate
uint8_t getSize(Gamestate*);

// Gets the amount of players in Gamestate
uint8_t getPlayerCount(Gamestate*);

// Gets the amount of trees in Gamestate
uint8_t getTreeCount(Gamestate*);

// Produces string that contains all data about players
int printPlayers(Gamestate*);

// Parses the data that gets sent to the players
int parseGamestate(Gamestate*, void*, int);

#endif
