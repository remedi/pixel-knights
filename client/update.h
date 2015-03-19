#ifndef UPDATE_H
#define UPDATE_H

#include <signal.h>

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

typedef struct mapdata {
  uint8_t height;
  uint8_t width;
  char **map;
} Mapdata;

// Context struct for thread
struct context_s {
    int* sock;
    pthread_mutex_t* lock;
    volatile sig_atomic_t* done;
};

// Thread cleanup handler
void free_memory(void *);

// Create map by allocating memory and adding walls
char **createMap(struct mapdata *);

// Add the new message contained in buf to the msg_array.
// Also rotate pointers to make the newest message show as first.
char **new_message(char *, char **, int); 

// ???
Playerdata *initGame(char *, Gamedata *, Mapdata);
 
// This function is a starting point for a thread.
// The point of this function is to read the messages received from server
// and to draw the game on the terminal
void *updateMap(void *);

#endif
