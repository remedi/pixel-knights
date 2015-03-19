#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>

#include "typedefs.h"

// Returns the maximum of a and b
int max(int, int);

// Create new ID for new client connection
ID createID(void);

// Check that the coordinate is valid 
int checkCoordinate(int);

// TODO: Load map data to server
int loadMap(FILE*);

#endif
