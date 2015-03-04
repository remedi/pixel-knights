#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "server.h"

// Forward declaration and typedef of gamestate_s
typedef struct gamestate_s Gamestate;

// Gamedata structure is a linked list
struct gamestate_s {
	ID id;
    Coord c;
    char sign;
	Gamestate* next;
};

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
