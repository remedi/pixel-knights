#ifndef TYPEDEF_H
#define TYPEDEF_H

#include <stdint.h>
#include <signal.h>

// ID is 8 bits long unsigned integer
typedef uint8_t ID;

typedef struct gamestate_s Gamestate;

typedef struct coodinate_s {
	uint8_t x;
	uint8_t y;
}Coord;

// Gamedata structure is a linked list (server side)
struct gamestate_s {
	ID id;
    Coord c;
    char sign;
    int sock;
    Gamestate* next;
};

//Playerdata on client side. These are about to be removed
typedef struct playerdata {
  char name[10];
  uint8_t id;
  uint8_t x_coord;
  uint8_t y_coord;
  uint8_t hp;
  uint8_t sign;
} Playerdata;

typedef struct gamedata {
  uint8_t player_count;
  uint8_t monster_count;
  Playerdata *players;
} Gamedata;


// Context structs for thread
typedef struct contex_server {
    int* sock;
    pthread_mutex_t* lock;
    volatile sig_atomic_t* done;
    int *main_exit;
}Contex_client_thread;

typedef struct contex_client {
    Gamestate* g;
    pthread_mutex_t* lock;
}Contex_server_thread;

// Action enumeration
typedef enum {
	UP,
	DOWN,
	RIGHT,
	LEFT,
	ATTACK,
	SHOOT
} Action;

typedef struct mapdata {
  uint8_t height;
  uint8_t width;
  char **map;
}Mapdata;


#endif