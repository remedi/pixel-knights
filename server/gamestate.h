#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "typedefs.h"

// Adds player to the gamestate linked-list
int addPlayer(Gamestate*, ID, Coord, char);

// Finds pointer to the Gamestate struct containing ID
Gamestate* findPlayer(Gamestate*, ID);

// Moves player to the destination Coord
int movePlayer(Gamestate*, ID, Coord);

// Changes the sign of the player
int changePlayerSign(Gamestate*, ID, char);

// Removes player from the gamestate linked-list
int removePlayer(Gamestate*, ID);

// Produces string that contains all data about players
int printPlayers(Gamestate*);

#endif
