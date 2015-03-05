#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>

#include "typedefs.h"

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
