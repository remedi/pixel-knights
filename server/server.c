// This file defines the functions declared in server.h
//
// Authors: Pixel Knights
// Date: 6.3.2015

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "../maps/maps.h"
#include "../address.h"
#include "server.h"
#include "gamestate.h"

// Spawns a new score point to the game in random place
int spawnScorePoint(Gamestate* g, Mapdata* m) {
    Coord random;
    int status;

    if (getScorePointCount(g) > 5) {
        // Don't spawn a new tree if there is 'too many'.
        // Return with no error
        return 0;
    }

    // Generate random coordinates for tree
    if ((status = randomCoord(g, m, &random))) {
        return status;
    }

    // Add point object
    if ((status = addObject(g, createID(g), random, -1, '$', POINT))) {
        return status;
    }

    return 0;
}


//Generate random coordinates, that are not occupied by anything at the map
int randomCoord(Gamestate *g, Mapdata *m, Coord *c) {

    Coord random;

    if(!g || !m) {
        return -1;
    }
    //Dont check for game collision if gamestate is empty
    if(g->next == NULL) {
        c->x = 1;
        c->y = 1;
        return 0;
    }

    //Two types of collisions to be avoided: collision with a element in gamestate, or with wall in mapdata
    int game_col = 0, map_col = 1;
    while(game_col || map_col) {
        random.x = rand() % m->width;
        random.y = rand() % m->height;
        map_col = checkWall(m, random);
        game_col = checkCollision(g, random);
    }
    c->x = random.x;
    c->y = random.y;
    return 0;
}

// Announce map server to matchmaking server
int registerToMM(char *MM_IP, char *MM_port, char map_nr, struct sockaddr_in* my_IP, struct sockaddr_in6* my_IP6) {
    int sock;
    struct sockaddr_in sock_addr_in;
    struct sockaddr_in6 sock_addr_in6;
    char message[2];
    socklen_t my_IP_len = sizeof(struct sockaddr_in);
    socklen_t my_IP6_len = sizeof(struct sockaddr_in6);
    int IP4;

    // The message sent to MM server contains 'S' + map number
    message[0] = 'S';
    message[1] = map_nr;

    // Parse address and port
    IP4 = isIpv4(MM_IP);
    if(IP4) {
        sock_addr_in = ipv4_parser(MM_IP, MM_port);
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("MM socket, create");
            return -1;
        }
        if (connect(sock, (struct sockaddr *) &sock_addr_in, sizeof(sock_addr_in)) == -1) {
            perror("MM socket, connect");
            return -1;
        }
    }
    else {
        sock_addr_in6 = ipv6_parser(MM_IP, MM_port);
        if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
            perror("MM socket, create");
            return -1;
        }
        if (connect(sock, (struct sockaddr *) &sock_addr_in6, sizeof(sock_addr_in6)) == -1) {
            perror("MM socket, connect");
            return -1;
        }
    }

    // Send server information
    if (write(sock, message, 2) == -1) {
        perror("MM socket, write");
        return -1;
    }
    // Wait for ok reply
    if (read(sock, message, 1) == -1) {
        perror("MM socket, read");
        return -1;
    }

    // OK reply from MM
    if (message[0] == 'O') {
        if(IP4) {
            if (getsockname(sock, (struct sockaddr *) my_IP, &my_IP_len) == -1) {
                perror("getsockname");
                return -1;
            } 
        }
        else {
            if (getsockname(sock, (struct sockaddr *) my_IP6, &my_IP6_len) == -1) {
                perror("getsockname");
                return -1;
            } 
        }
        close(sock);
        return 0;
    }
    // Something went wrong
    else {
        close(sock);
        return -2;
    }
}

// Returns the maximum of a and b
int max(int a, int b) {

    if (a > b)
        return a;
    return b;
}

// Returns the ID of a new client connection
ID createID(Gamestate* g) {

    static ID id = 0x01;

    if (!id)
        id++;

    while (findObject(g, id))
        ++id;

    return id++;
}

// Checks for player collisions in target coordinate
int checkCollision(Gamestate* g, Coord c) {

    // If Gamestate NULL
    if (!g)
        return -1;

    // Linked-list is empty
    if (g->next == NULL)
        return -2;

    // First element does not contain player
    g = g->next;

    while(g) {
        if (g->c.x == c.x && g->c.y == c.y) {
            // Collision with a bullet
            if(g->type == BULLET) {
                return -4;
            }
            // Collision with a point
            else if(g->type == POINT) {
                return -5;
            }
            // Collision with player
            else {
                return -3;
            }
        }
        g = g->next;
    }
    // No collision
    return 0;
}

// Sends announcement to all players
int sendAnnounce(Gamestate* g, char* msg, size_t len, ID id) {

    // If gamestate NULL
    if (!g)
        return -1;

    // Linked-list is empty
    if (g->next == NULL)
        return -2;

    // First element does not contain player
    g = g->next;

    // Send announcement to all players
    while (g != NULL) {
        if (g->id == id) {
            g = g->next;
            continue;
        }
        // Only send to player objects
        if (g->type == PLAYER) {
            if (send(g->data, msg, len, 0) < 0) {
                perror("send error");
                return -3;
            }
        }
        g = g->next;
    }
    return 0;
}

