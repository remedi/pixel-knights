#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>

#include "../typedefs.h"

// Send game state to all connected clients periodically
void* sendGamestate(void*);

#endif
