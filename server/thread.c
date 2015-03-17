// This file defines the game state sending thread
//
// Authors: Pixel knights
// Date: 17.3.2015

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>

#include "gamestate.h"
#include "thread.h"
#include "typedefs.h"

#define MAXDATASIZE 128
#define G_INTERVAL 5

void* sendGamestate(void* ctx) {

    // Init variables
    char sendbuf[MAXDATASIZE];
    struct context_s* c = (struct context_s*) ctx;
    Gamestate* g = c->g;
    Gamestate* game = g;
    int status, nbytes;

    // Keep sending the game state to clients until thread_cancel()
    while(1) {

        // Thread keeps sending the updates with G_INTERVAL seconds interval
        sleep(G_INTERVAL);

        // Clear the send buffer
        memset(sendbuf, 0, MAXDATASIZE);

        // Lock the game state for reading
        pthread_mutex_lock(c->lock);

        // Parse game state message
        status = parseGamestate(game, sendbuf, MAXDATASIZE);

        if (status == -1) {
            fprintf(stderr, "Thread: Invalid game state\n");
            pthread_mutex_unlock(c->lock);
            continue;
        }
        else if (status == -2) {
            fprintf(stderr, "Thread: Empty game\n");
            pthread_mutex_unlock(c->lock);
            continue;
        }
        else if (status == -3) {
            fprintf(stderr, "Thread: Not enough memory for message\n");
            pthread_mutex_unlock(c->lock);
            continue;
        }

        // Send game state to every client
        g = game;
        while (g->next != NULL) {
            printf("%d", g->sock);
            if ((nbytes = send(g->next->sock, sendbuf, status, 0)) == -1) {
                perror("Thread: send error");
            }
            g = g->next;
        }

        // Release the lock
        pthread_mutex_unlock(c->lock);
    }
}
