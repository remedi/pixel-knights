#ifndef TYPEDEF_H
#define TYPEDEF_H

#include <stdint.h>

// ID is 8 bits long unsigned integer
typedef uint8_t ID;

// Coordinate struct
typedef struct coodinate_s {
	uint8_t x;
	uint8_t y;
} Coord;

typedef struct gamestate_s Gamestate;

// Gamedata structure is a linked list
struct gamestate_s {
	ID id;
    Coord c;
    char sign;
    int sock;
    Gamestate* next;
};

// Action enumeration
typedef enum {
	UP,
	DOWN,
	RIGHT,
	LEFT,
	ATTACK,
	SHOOT
} Action;

#endif
