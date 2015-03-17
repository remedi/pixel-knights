#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>

#include "typedefs.h"

struct context_s {
    Gamestate* g;
    pthread_mutex_t* lock;
};

void* sendGamestate(void*);

#endif
