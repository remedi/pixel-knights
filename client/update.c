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

#define BUFLEN 1000
#define MSGLEN 40

#define G_OFFSET_C 1
#define G_OFFSET_P 4

// Frees memory pointed by 'ptr'. Used as thread cleanup handler
void free_memory(void *ptr) {
    //printf("thread: Freeing my memory\n");
    free(ptr);
}

// Add the new message contained in buf to the msg_array.
// Also rotate pointers to make the newest message show as first.
void new_message(char *buf, char **msg_array, int msg_count) {
    
    // Move all messages up by 1
    char *first_msg = msg_array[0];
    for (int i = 0; i < msg_count-1; i++) {
        msg_array[i] = msg_array[i+1];
    }
    msg_array[msg_count-1] = first_msg;
    
    // Add newest message as first
    memset(msg_array[msg_count-1], '\0', MSGLEN);
    memcpy(msg_array[msg_count-1], buf, MSGLEN);
}

// This function is a starting point for a thread.
// The point of this function is to read the messages received from server
// and to draw the game on the terminal
void *updateMap(void *ctx) {
    Context_client_thread* c = (Context_client_thread *) ctx;
    char *map_nr = c->map_nr;
    Mapdata map_data;
    ssize_t bytes;
    int break_flag = 1;
    char *buf = malloc(sizeof(char) * BUFLEN);
    char **message_array;
    int message_count;
    int i, draw_count = 0;

    // Pointers for iteration
    uint8_t* player_count,* x,* y;
    char* sign;

    // Initialize mapdata
    if (createMap(&map_data, *map_nr) == -1) {
        printf("thread: Error with createMap\n");
    }

    //Reserve memory for chat service
    message_count = map_data.height;
    message_array = malloc(sizeof(char*) * message_count);
    for (i = 0; i < message_count; i++) {
        message_array[i] = malloc(sizeof(char) * MSGLEN);
        memset(message_array[i], '\0', MSGLEN);
    }

    //printf("thread: Reading socket %d\n", *c->sock);
    while (break_flag && !*c->main_exit) {
        // Clean message buffer
        memset(buf, '\0', BUFLEN);

        // Block here until message comes
        bytes = read(*c->sock, buf, BUFLEN);
        if (bytes < 0) {
            perror("read");
            continue;
        }
        else if (bytes == 0) {
            printf("thread: Socket closed unexpectedly\n");
            break_flag = 0;
            continue;
        }
        else if (buf[0] == 'C') {
            new_message(buf+1, message_array, message_count);
            continue;
        }
        else if (buf[0] == 'G') {
            player_count = (uint8_t*) buf+G_OFFSET_C;
            x = (uint8_t*) buf+G_OFFSET_C+2;
            y = (uint8_t*) buf+G_OFFSET_C+3;
            sign = (char*) buf+G_OFFSET_C+4;
        }
        else {
            printf("thread: Received unkown message: %c. Ignoring\n", buf[0]);
            continue;
        }

        // Only draw map if user isn't busy doing something else
        if (!pthread_mutex_trylock(c->lock)) {
            system("clear");

            // Add players to the map
            for (i = 0; i < *player_count; i++) {
                map_data.map[*y][*x] = *sign;
                x += G_OFFSET_P;
                y += G_OFFSET_P;
                sign += G_OFFSET_P;
            }
	    //Print 1 newline in order to start drawing from the second line:
	    printf("\n");
            for (i = 0; i < map_data.height; i++) {
                //Actual drawing
                printf("\t%s", map_data.map[i]);
                printf("\t\t%s\n", message_array[i]);
            }
            x = (uint8_t*) buf+G_OFFSET_C+2;
            y = (uint8_t*) buf+G_OFFSET_C+3;
            // Remove players from the map
            for (i = 0; i < *player_count; i++) {
                map_data.map[*y][*x] = ' ';
                x += G_OFFSET_P;
                y += G_OFFSET_P;
                sign += G_OFFSET_P;
            }

            printf("\nDEBUG: Drawing loop: %d. Press 'h' for help\n", draw_count++);
            pthread_mutex_unlock(c->lock);
        }
    }

    free_memory(buf);
    freeMap(&map_data);

    // Free memory allocated for messages
    for(i = 0; i<message_count; i++) {
        free(message_array[i]);
    }
    free(message_array);

    *c->main_exit = 1;
    //printf("thread: Exiting...\n");
    return 0;
}
