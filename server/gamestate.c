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


//Spawn a new tree 'character' in a random place
int spawnTree(Gamestate* g, Mapdata* m) {
    Coord random;
    int status;

    status = getTreeCount(g);
    if(status > 10) {
        // Don't spawn a new tree if there is 'too many'. Return with no error
        return 0;
    }

    //Generate random coordinates for tree
    status = randomCoord(g, m, &random);
    if(status) {
        return status;
    }

    //Create new tree 'character'. Sock -2 means that it is tree
    status = addPlayer(g, createID(g), random, -2, '$', 3);
    if(status) {
        return status;
    }

    return 0;
}


// Adds player to the gamestate linked-list
int addPlayer(Gamestate* g, ID id, Coord c, int sock, char sign, char data) {

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
    player->sock = sock;
    player->data = data;
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

// Gets the amount of elements in Gamestate
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

// Gets the amount of players in Gamestate
uint8_t getPlayerCount(Gamestate* g) {

    uint8_t size = 0;

    // First element does not contain player
    g = g->next;

    // Iterate all the elements
    while (g != NULL) {
        if(g->sock > 0) 
            size++;
        g = g->next;
    }
    return size;
}

// Gets the amount of trees in Gamestate
uint8_t getTreeCount(Gamestate* g) {

    uint8_t size = 0;

    // First element does not contain player
    g = g->next;

    // Iterate all the elements
    while (g != NULL) {
        if(g->sock == -2) 
            size++;
        g = g->next;
    }
    return size;
}

// Add bullet 'character' next to a player
int addBullet(Gamestate *g, Mapdata *map_data, ID id, Action a) {
    Coord temp_coord;
    Gamestate* game = g;

    // If gamestate NULL
    if (!g)
        return -1;

    // Iterate the linked-list
    if (!(g = findPlayer(g, id)))
        return -2;

    temp_coord = g->c;

    switch(a) {
        case SHOOT_RIGHT:
            temp_coord.x++;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    if(addPlayer(game, createID(g), temp_coord, -1, '*', RIGHT) != 0) {
                        return -5;
                    }
                    break;
                }
            }
            return -3;

        case SHOOT_LEFT:
            temp_coord.x--;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    if(addPlayer(game, createID(g), temp_coord, -1, '*', LEFT) != 0) {
                        return -5;
                    }
                    break;
                }
            }
            return -3;

        case SHOOT_UP:
            temp_coord.y--;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    if(addPlayer(game, createID(g), temp_coord, -1, '*', UP) != 0) {
                        return -5;
                    }
                    break;
                }
            }
            return -3;

        case SHOOT_DOWN:
            temp_coord.y++;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    if(addPlayer(game, createID(g), temp_coord, -1, '*', DOWN) != 0) {
                        return -5;
                    }
                    break;
                }
            }
            return -3;

        default:
            return -4;
    }
    return 0;
}

// Move every bullet once to direction that bullet is heading.
// This direction is determined by the Action enum in the bullets data field
int updateBullets(Gamestate* g, Mapdata *map_data) {

    Gamestate* game = g;

    // If gamestate NULL
    if (!g)
        return -1;

    while(g != NULL) {
        if(g->sock == -1) {
            //Bullets move as normal players
            if(movePlayer(game, map_data, g->id, g->data) == -3) {
                //If bullet collides with anything, remove it
                removePlayer(game, g->id);
            }
        }
        g = g->next;
    }
    return 0;
}

// This function performs the action that player has requested: Move player or shoot a bullet.
int movePlayer(Gamestate* g, Mapdata *map_data, ID id, Action a) {

    Coord temp_coord;
    Gamestate* game = g;

    // If gamestate NULL
    if (!g)
        return -1;

    // Iterate the linked-list
    if (!(g = findPlayer(g, id)))
        return -2;

    temp_coord = g->c;

    // Update coordinates
    switch(a) {
        case UP:
            temp_coord.y--;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    g->c.y--;
                    break;
                }
            }
            return -3;

        case DOWN:
            temp_coord.y++;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    g->c.y++;
                    break;
                }
            }
            return -3;

        case LEFT:
            temp_coord.x--;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    g->c.x--;
                    break;
                }
            }
            return -3;

        case RIGHT:
            temp_coord.x++;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    g->c.x++;
                    break;
                }
            }
            return -3;

        case SHOOT_RIGHT:
            return addBullet(game, map_data, id, a);

        case SHOOT_LEFT:
            return addBullet(game, map_data, id, a);

        case SHOOT_DOWN:
            return addBullet(game, map_data, id, a);

        case SHOOT_UP:
            return addBullet(game, map_data, id, a);

        default:
            return -4;
    }
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
        printf("%02x: (%hhu,%hhu) - %c @ %d\n", g->id, g->c.x, g->c.y, g->sign, g->sock);
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

// Terminates the Gamestate instance
void freePlayers(Gamestate* g) {

    // If gamestate NULL
    if (!g)
        return;

    // First element does not contain player
    g = g->next;

    // Free all memory
    Gamestate* tmp; 
    while (g) {
        tmp = g->next;
        free(g);
        g = tmp;
    }
}
