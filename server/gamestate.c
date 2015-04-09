// This file defines the functions declared in gamestate.h
// 
// Authors: Pixel Knights
// Date: 4.2.2015

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../maps/maps.h"
#include "gamestate.h"
#include "server.h" 


// Adds object to the gamestate linked-list
int addObject(Gamestate* g, ID id, Coord c, int data, char sign, Type t, char* name) {

    // If gamestate NULL
    if (!g)
        return -1;

    // If ID is already in the game
    if (findObject(g, id))
        return -2;

    // Find the last element of linked list
    while (g->next != NULL)
        g = g->next;

    // Allocate memory for object and assign values
    Gamestate* object = malloc(sizeof(Gamestate));
    object->id = id;
    object->c = c;
    object->sign = sign;
    object->data = data;
    object->type = t;
    object->score = 0;

    // Allocate memory for the player name
    if (object->type == PLAYER) {
        char* n = malloc(sizeof(char) * (strlen(name)+1));
        strcpy(n, name);
        n[strlen(name)] = '\0';
        object->name = n;
    }
    else
        object->name = NULL;

    // Add the object to the linked-list
    object->next = NULL;
    g->next = object;

    return 0;
}

// Finds pointer to the Gamestate struct containing ID
Gamestate* findObject(Gamestate* g, ID id) {

    // If gamestate NULL
    if (!g)
        return NULL;

    // Find the right object
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

// Gets the amount of elements in Gamestate
uint8_t getSize(Gamestate* g) {

    uint8_t size = 0;

    // First element does not contain object
    g = g->next;

    // Iterate all the elements
    while (g != NULL) {
        size++;
        g = g->next;
    }
    return size;
}

// Gets the amount of players in Gamestate
uint8_t getPlayerCount(Gamestate* g) {

    uint8_t size = 0;

    // First element does not contain player
    g = g->next;

    // Iterate all the elements
    while (g != NULL) {
        if (g->type == PLAYER) 
            size++;
        g = g->next;
    }
    return size;
}

// Gets the amount of score points in Gamestate
uint8_t getScorePointCount(Gamestate* g) {

    uint8_t size = 0;

    // First element does not contain object
    g = g->next;

    // Iterate all the elements
    while (g != NULL) {
        if (g->type == POINT) 
            size++;
        g = g->next;
    }
    return size;
}

// Changes the sign of the player
int changePlayerSign(Gamestate* g, ID id, char sign) {

    // If gamestate NULL
    if (!g)
        return -1;

    // Iterate the linked-list
    if (!(g = findObject(g, id)))
        return -2;

    // Update sign
    g->sign = sign;

    return 0;
}

// Removes the player from the gamestate linked-list
Gamestate* removeObject(Gamestate* g, ID id) {

    // If gamestate NULL
    if (!g)
        return NULL;

    while (g->next != NULL) {
        // ID found
        if (g->next->id == id) {
            Gamestate* tmp = g->next->next;
            // If Object has a name
            if (g->next->name)
                free(g->next->name);
            free(g->next);
            g->next = tmp;
            return g;
        }
        g = g->next;
    }

    // Not found
    return NULL;
}

// Print for debugging purposes
int printObjects(Gamestate* g) {

    // If gamestate NULL
    if (!g)
        return -1;

    // Linked-list is empty
    if (g->next == NULL)
        return -2;

    // First element does not contain object
    g = g->next;
    printf("Current objects:\n");
    while (g != NULL) {
        if (g->name)
            printf("%02x: (%hhu,%hhu) - %c @ %s\n", g->id, g->c.x, g->c.y, g->sign, g->name);
        else
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

    // First element does not contain object
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

// Terminates the Gamestate instance
void freeGamestate(Gamestate* g) {

    // If gamestate NULL
    if (!g)
        return;

    // First element does not contain object
    g = g->next;

    // Free all memory
    Gamestate* tmp; 
    while (g) {
        tmp = g->next;
        if (g->name)
            free(g->name);
        free(g);
        g = tmp;
    }
}
