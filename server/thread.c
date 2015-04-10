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
#include "server.h"
#include "thread.h"

#define MAXDATASIZE 1026
#define G_INTERVAL_USEC 100000

//Bullets are moved according to this value, smaller the faster: 
#define BULLET_INTERVAL 1

//Trees are spawned according to this value, smaller the faster: 
#define TREE_INTERVAL 100

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
        g = game->next;
        while (g != NULL) {
            // Send only to player objects
            if (g->type == PLAYER) {
                if ((nbytes = send(g->data, sendbuf, status, 0)) == -1) {
                    perror("Thread: send error");
                }
            }
            g = g->next;
        }

        // Move bullets every BULLET_INTERVAL 'th loop
        if(++i % BULLET_INTERVAL == 0) {
            updateBullets(game, m);
        }

        // Spawn score point, but don't spawn if game is empty
        if(i % TREE_INTERVAL == 0) {
            if(getPlayerCount(game)) {
                spawnScorePoint(game, m);
            }
        }

        // Release the lock
        pthread_mutex_unlock(c->lock);
    }
}
