#ifndef UPDATE_H
#define UPDATE_H

#include <signal.h>
#include "../typedefs.h"

// Thread cleanup handler
void free_memory(void *);

// Add the new message contained in buf to the msg_array.
// Also rotate pointers to make the newest message show as first.
char **new_message(char *, char **, int); 

// This function is a starting point for a thread.
// The point of this function is to read the messages received from server
// and to draw the game on the terminal
void *updateMap(void *);

#endif
