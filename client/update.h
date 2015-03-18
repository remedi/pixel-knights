#ifndef UPDATE_H
#define UPDATE_H

// Context struct for thread
struct context_s {
    int* sock;
    pthread_mutex_t* lock;
};

// Add the new message contained in buf to the msg_array.
// Also rotate pointers to make the newest message show as first.
char **new_message(char *buf, char **msg_array, int msg_count); 

// ???
Playerdata *initGame(char *buf, Gamedata *game_data, Mapdata map_data);
 
// This function is a starting point for a thread.
// The point of this function is to read the messages received from server
// and to draw the game on the terminal
void *updateMap(void *arg);

#endif
