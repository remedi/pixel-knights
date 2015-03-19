#ifndef MAPS_H
#define MAPS_H

typedef struct mapdata {
  uint8_t height;
  uint8_t width;
  char **map;
}Mapdata;

typedef struct coodinate_s {
	uint8_t x;
	uint8_t y;
} Coord;

//Reserve memory for map and initialize tiles, return pointer for the reserved memory
int createMap(Mapdata *, int);

//Checks if there is wall in the coordinates. Will return 0 if not.
int checkWall(Mapdata *, Coord);

//Frees memory allocated for map.
void freeMap(Mapdata *);

#endif
