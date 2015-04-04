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

#define MAXDATASIZE 128
#define G_INTERVAL_USEC 100000

// Send game state to all connected clients periodically
void* sendGamestate(void* ctx) {

    // Init variables
    char sendbuf[MAXDATASIZE];
    Context_server_thread* c = (Context_server_thread *) ctx;
    Gamestate* g = c->g;
    Mapdata* m = c->m;
    Gamestate* game = g;
    int status, nbytes, i = 0;

    // Keep sending the game state to clients until thread_cancel()
    while(1) {
        // Thread keeps sending the updates with G_INTERVAL seconds interval
        usleep(G_INTERVAL_USEC);

        // Clear the send buffer
        memset(sendbuf, 0, MAXDATASIZE);

        // Lock the game state for reading
        pthread_mutex_lock(c->lock);

	// Move bullets once every second so they can still be debugged.
	i++;
	if(i % 10 == 0) {
	    updateBullets(game, m);
	}


        // Parse game state message
        status = parseGamestate(game, sendbuf, MAXDATASIZE);

        if (status == -1) {
            fprintf(stderr, "Thread: Invalid game state\n");
            pthread_mutex_unlock(c->lock);
            continue;
        }
        else if (status == -2) {
            // fprintf(stderr, "Thread: Empty game\n");
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
	    //Don't send to bullets etc
	    if(g->next->sock == -1) {
		g = g->next;
		continue;
	    }
            if ((nbytes = send(g->next->sock, sendbuf, status, 0)) == -1) {
                perror("Thread: send error");
            }
            g = g->next;
        }

        // Release the lock
        pthread_mutex_unlock(c->lock);
    }
}
