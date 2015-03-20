// This file defines the functions of update thread that reads messages from the server
//
// Authors: Pixel knights
// Date: 18.3.2015

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "../maps/maps.h"
#include "client.h"
#include "update.h"

#define MAP_HEIGHT 10
#define MAP_WIDTH 10
#define BUFLEN 1000
#define MSGLEN 40

// Frees memory pointed by 'ptr'. Used as thread cleanup handler
void free_memory(void *ptr) {
    printf("thread: Freeing my memory\n");
    free(ptr);
}

// Add the new message contained in buf to the msg_array.
// Also rotate pointers to make the newest message show as first.
char **new_message(char *buf, char **msg_array, int msg_count) {
    int i;
    char *temp_ptr = msg_array[msg_count-1];
    //Rearrange pointers
    for(i = (msg_count-1); i>0; i--) {
        msg_array[i] = msg_array[i-1];
    }
    msg_array[0] = temp_ptr;
    memset(msg_array[0], '\0', MSGLEN);
    //Add newest message as first
    memcpy(msg_array[0], buf, MSGLEN);
    return msg_array;
}

// This function is called when G message is received.
// Parse game status from a character string received from the server.
Playerdata *initGame(char *buf, Gamedata *game_data, Mapdata map_data) {
    buf++;
    int i;
    int height = map_data.height;
    int width = map_data.width;
    Playerdata *players = game_data->players;
    Playerdata *old_ptr = players;
    game_data->player_count = *buf;
    if(game_data->player_count < 1) {
        printf("Sanity check failed: player_count was: %d\n", game_data->player_count);
        free(players);
        return NULL;
    }
    buf++;
    players = realloc(players, game_data->player_count * sizeof(Playerdata));
    if(players == NULL) {
        //free(players);
        free(old_ptr);
        perror("realloc");
        return NULL;
    }
    for(i = 0; i < game_data->player_count; i++) {
        players[i].id = *buf;
        buf++;
        players[i].x_coord = *buf;
        if(players[i].x_coord > width || players[i].x_coord < 1) {
            printf("thread: G message failed sanity check. Player coordinates extend beyond map. X: %d\n", *buf);
            /*free(players);
              return NULL;*/
        }
        buf++;
        players[i].y_coord = *buf;
        if(players[i].y_coord > height || players[i].y_coord < 1) {
            printf("thread: G message failed sanity check. Player coordinates extend beyond map. Y: %d\n", *buf);
            /*free(players);
              return NULL;*/
        }
        buf++;
        players[i].sign = *buf;
        buf++;
    }
    return players;
}

// This function is a starting point for a thread.
// The point of this function is to read the messages received from server
// and to draw the game on the terminal
void *updateMap(void *ctx) {
    Gamedata game;
    Contex_client_thread* c = (Contex_client_thread *) ctx;
    int *sock_t = c->sock;
    Playerdata *players = malloc(sizeof(Playerdata));
    Mapdata map_data;
    game.players = players;
    uint8_t x, y;
    int i, j = 0;
    ssize_t bytes;
    int break_flag = 1;
    char *buf = malloc(sizeof(char) * BUFLEN);
    char **rows;
    char **message_array;
    int message_count;
    int *exit_clean = c->main_exit;

    // Initialize mapdata
    if(createMap(&map_data, 2) == -1) {
	printf("thread: error with createMap\n");
	exit(EXIT_FAILURE);
    }
    rows = map_data.map;

    //Reserve memory for chat service
    message_count = MAP_HEIGHT;
    message_array = malloc(sizeof(char *) * message_count);
    for(i = 0; i<message_count; i++) {
        message_array[i] = malloc(sizeof(char) * MSGLEN);
        memset(message_array[i], '\0', MSGLEN);
    }

    // Clean message buffer
    memset(buf, '\0', BUFLEN);

    //Cleanup handlers for thread cancellation
    printf("thread: Reading socket %d\n", *sock_t);
    while(break_flag && ! *exit_clean) {
        // This cleanup handler needs to be set again for each loop
        // in case the address of 'players' is changed.
        bytes = read(*sock_t, buf, BUFLEN);
        if(bytes < 0) {
            perror("read");
            continue;
        }
        else if(bytes == 0) {
            printf("thread: Server likely disconnected\n");
            break_flag = 0;
        }
        else if(buf[0] == 'C') {
            printf("thread: Got C message\n");
            new_message(buf+1, message_array, message_count);
        }
        else if(buf[0] == 'G') {
            if((bytes - 2) % 4 != 0) {
                printf("G message failed sanity check. Unaccepted message length: %lu\n", bytes);
                memset(buf, '\0', BUFLEN);
                continue;
            }
            players = initGame(buf, &game, map_data);
            if(players == NULL) {
                printf("thread: initGame failed\n");
                game.player_count = 0;
            }
            game.players = players;
            memset(buf, '\0', BUFLEN);
        }
        else {
            printf("thread: Received unkown message: %c. Ignoring\n", buf[0]);
            continue;
        }

        // Only draw map if user isn't busy doing something else
        if(pthread_mutex_trylock(c->lock) == 0) {
            system("clear");
            for(i = 0; i<game.player_count; i++) {
                //Add players
                x = players[i].x_coord;
                y = players[i].y_coord;
                rows[y][x] = players[i].sign;
            }
            printf("\n");
            for(i = 0; i<map_data.height; i++) {
                //Actual drawing
                printf("\t%s", rows[i]);
                printf("\t\t%s\n", message_array[message_count-i-1]);
            }
            printf("\n");
            for(i = 0; i<game.player_count; i++) {
                //Undo changes
                x = players[i].x_coord;
                y = players[i].y_coord;
                rows[y][x] = ' ';
            }

            printf("DEBUG: Drawing loop: %d. Press 'h' for help\n", j);
            j++;
            pthread_mutex_unlock(c->lock);
        }
        memset(buf, '\0', BUFLEN);
    }

    free_memory(buf);
    freeMap(&map_data);
    //free_memory(players);
    // Free memory allocated for messages
    for(i = 0; i<message_count; i++) {
        free(message_array[i]);
    }
    free(message_array);

    *c->done = 1;
    free(players);
    printf("thread: Server hung up unexpectedly, exiting\n");
    return 0;
}
