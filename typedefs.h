#ifndef TYPEDEF_H
#define TYPEDEF_H

#include <stdint.h>
#include <pthread.h>
#include <signal.h>

// ID is 8 bits long unsigned integer
typedef uint8_t ID;

typedef struct coodinate_s {
    uint8_t x;
    uint8_t y;
}Coord;

// Forward declaration of Gamestate
typedef struct gamestate_s Gamestate;

// Gamedata structure is a linked list (server side)
struct gamestate_s {
    ID id;
    Coord c;
    char sign;
    int sock;
    Gamestate* next;
};

// Context struct for client thread
typedef struct context_client {
    int* sock;
    pthread_mutex_t* lock;
    volatile sig_atomic_t* done;
    int *main_exit;
    char *map_nr;
} Context_client_thread;

// Context struct for server thread
typedef struct context_server {
    Gamestate* g;
    pthread_mutex_t* lock;
} Context_server_thread;

// Action enumeration
typedef enum {
    UP,
    DOWN,
    RIGHT,
    LEFT,
    ATTACK,
    SHOOT
} Action;

// Data structure for map loaded from a file
typedef struct mapdata {
    uint8_t height;
    uint8_t width;
    uint8_t map_nr;
    char **map;
} Mapdata;

#endif
