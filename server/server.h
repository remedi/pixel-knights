#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <stdio.h>

#include "gamestate.h"

// ID is 8 bits long unsigned identifier
typedef uint8_t ID;

// Action enumeration
typedef enum {
	UP,
	DOWN,
	RIGHT,
	LEFT,
	ATTACK,
	SHOOT
} Action;

// Coordinate struct
typedef struct coodinate_s {
	uint8_t x;
	uint8_t y;
} Coord;

// Send the game state to the clients
int sendGameState(void);

// Receive Action enum from client
Action receiveAction(unsigned int);

// Receive chat message from client
char* receiveChatMessage(unsigned int);

// Relay chat message to client
int relayChatMessage(char*, unsigned int);

// Listen for new connections
int listenNewConnections(void);

// Perform id negotiation on new connection
ID negotiateID(int);

// Load map data to server
int loadMap(FILE*);

#endif
