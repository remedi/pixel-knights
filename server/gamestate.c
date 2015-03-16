// This file defines the functions declared in gamestate.h
// 
// Authors: Pixel Knights
// Date: 4.2.2015

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gamestate.h"

// Adds player to the gamestate linked-list
int addPlayer(Gamestate* g, ID id, Coord c, char sign) {

    // If gamestate NULL
    if (!g)
        return -1;

    // If ID is already in the game
    if (findPlayer(g, id))
        return -2;

    // Find the last element of linked list
    while (g->next != NULL)
        g = g->next;

    // Allocate memory for player and assign values
    Gamestate* player = malloc(sizeof(Gamestate));
    player->id = id;
    player->c = c;
    player->sign = sign;
    player->next = NULL;
    g->next = player;

    return 0;
}

// Finds pointer to the Gamestate struct containing ID
Gamestate* findPlayer(Gamestate* g, ID id) {

    // If gamestate NULL
    if (!g)
        return NULL;

    // Find the right player
    while (g->next != NULL) {
        if (g->id == id)
            break;
        g = g->next;
    }

    // ID not found
    if (g->next == NULL && g->id != id)
        return NULL;

    return g;
}

// Gets the amount of players in Gamestate
uint8_t getSize(Gamestate* g) {

    uint8_t size = 0;

    // First element does not contain player
    g = g->next;

    // Iterate all the elements
    while (g != NULL) {
        size++;
        g = g->next;
    }
    return size;
}

// Moves plyer to the destination Coord
int movePlayer(Gamestate* g, ID id, Coord c) {

    // If gamestate NULL
    if (!g)
        return -1;
    
    // Iterate the linked-list
    if (!(g = findPlayer(g, id)))
        return -2;

    // Update coordinates
    g->c = c;

    return 0;
}

// Changes the sign of the player
int changePlayerSign(Gamestate* g, ID id, char sign) {

    // If gamestate NULL
    if (!g)
        return -1;

    // Iterate the linked-list
    if (!(g = findPlayer(g, id)))
        return -2;

    // Update sign
    g->sign = sign;

    return 0;
}

// Removes the player from the gamestate linked-list
int removePlayer(Gamestate* g, ID id) {

    // If gamestate NULL
    if (!g)
        return -1;

    while (g->next != NULL) {
        // ID found
        if (g->next->id == id) {
            Gamestate* tmp = g->next->next;
            free(g->next);
            g->next = tmp;
            return 0;
        }
        g = g->next;
    }

    // Not found
    return -2;
}

// Print for debugging purposes
int printPlayers(Gamestate* g) {

    // If gamestate NULL
    if (!g)
        return -1;

    // Linked-list is empty
    if (g->next == NULL)
        return -2;

    // First element does not contain player
    g = g->next;
    printf("Current players:\n");
    while (g != NULL) {
        printf("%02x: (%hhu,%hhu) - %c\n", g->id, g->c.x, g->c.y, g->sign);
        g = g->next;
    }
    printf("\n");

    // Done
    return 0;    
}

// Parses the string that gets sent to the players
int parseGamestate(Gamestate* g, void* s, int len) {

    uint8_t* data = s;

    // Index for memory iteration
    int i = 2;

    // If gamestate NULL
    if (!g)
        return -1;

    // Linked list is empty
    if (g->next == NULL)
        return -2;

    // The message format starts with a G and player amount
    data[0] = 0x47; // ASCII G
    data[1] = getSize(g);

    // First element does not contain player
    g = g->next;

    while (g != NULL) {

        // Not enough memory
        if (i + 4 > len)
            return -3;

        // Copy from struct gamestate
        memcpy(data + i, g, 4);
        i += 4;

        g = g->next;
    }

    // Done
    return i;
}
