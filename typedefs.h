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
} Coord;

// Gamestate object types
typedef enum {
    PLAYER,
    BULLET,
    POINT
} Type;

// Forward declaration of Gamestate
typedef struct gamestate_s Gamestate;

// Gamedata structure is a linked list
struct gamestate_s {
    ID id;
    Coord c;
    char sign;
    int data;
    Type type;
    char* name;
    uint8_t score;
    Gamestate* next;
    int udp_sock;
};

// Data structure for map loaded from a file
typedef struct mapdata {
    uint8_t height;
    uint8_t width;
    uint8_t map_nr;
    char **map;
} Mapdata;

// Context struct for client thread
typedef struct context_client {
    int* sock;
    pthread_mutex_t* lock;
    int *main_exit;
    char *map_nr;
} Context_client_thread;

// Context struct for server thread
typedef struct context_server {
    Gamestate* g;
    Mapdata* m;
    pthread_mutex_t* lock;
} Context_server_thread;

// Action enumeration
typedef enum {
    UP,
    DOWN,
    RIGHT,
    LEFT,
    SHOOT_UP,
    SHOOT_DOWN,
    SHOOT_LEFT,
    SHOOT_RIGHT
} Action;

#endif
