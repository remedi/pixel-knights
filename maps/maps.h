#ifndef MAPS_H
#define MAPS_H

#include <stdint.h>
#include "../typedefs.h"

//Reserve memory for map and initialize tiles, return pointer for the reserved memory
int createMap(Mapdata *, int);

//Checks if there is wall in the coordinates. Will return 0 if not.
int checkWall(Mapdata *, Coord);

//Frees memory allocated for map.
void freeMap(void *);

#endif
