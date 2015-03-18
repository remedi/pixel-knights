#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <inttypes.h>

#include "typedefs.h"

// Adds player to the gamestate linked-list
int addPlayer(Gamestate*, ID, Coord, int, char);

// Finds pointer to the Gamestate struct containing ID
Gamestate* findPlayer(Gamestate*, ID);

// Gets the amount of players in Gamestate
uint8_t getSize(Gamestate*);

// Moves player to the destination Coord
int movePlayer(Gamestate*, ID, Action);

// Changes the sign of the player
int changePlayerSign(Gamestate*, ID, char);

// Removes player from the gamestate linked-list
int removePlayer(Gamestate*, ID);

// Produces string that contains all data about players
int printPlayers(Gamestate*);

// Parses the data that gets sent to the players
int parseGamestate(Gamestate*, void*, int);

// Sends announcement to all players except ID
int sendAnnounce(Gamestate*, char*, size_t, ID);

#endif
