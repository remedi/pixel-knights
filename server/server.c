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

#define MAXLINE 128

// This function performs the action that player has requested: Move player or shoot a bullet.
int processAction(Gamestate* g, Mapdata *map_data, ID id, Action a) {
    Coord temp_coord;
    Gamestate* game = g;
    int status = 0;
    ID collision = 0x00;
    char sendbuf[MAXLINE];

    // If gamestate NULL
    if (!g)
        return -1;

    // Iterate the linked-list
    if (!(g = findObject(g, id)))
        return -2;

    temp_coord = g->c;

    // Select action, if collision occurs
    // the collided ID is stored in collision
    switch(a) {

        // Move the object if action is just a movement
        case UP:
            temp_coord.y--;
            if (!(status = checkWall(map_data, temp_coord))) {
                if (!(collision = checkCollision(game, temp_coord)))
                    g->c.y--;
            }
            break;

        case DOWN:
            temp_coord.y++;
            if (!(status = checkWall(map_data, temp_coord))) {
                if (!(collision = checkCollision(game, temp_coord)))
                    g->c.y++;
            }
            break;

        case LEFT:
            temp_coord.x--;
            if (!(status = checkWall(map_data, temp_coord))) {
                if (!(collision = checkCollision(game, temp_coord)))
                    g->c.x--;
            }
            break;

        case RIGHT:
            temp_coord.x++;
            if (!(status = checkWall(map_data, temp_coord))) {
                if (!(collision = checkCollision(game, temp_coord)))
                    g->c.x++;
            }
            break;

        // Create new bullet if the action is shooting
        case SHOOT_RIGHT:
            temp_coord.x++;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    if(!addObject(game, createID(g), temp_coord, RIGHT, '*', BULLET, NULL))
                        break;
                }
            }
            return -5;

        case SHOOT_LEFT:
            temp_coord.x--;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    if(!addObject(game, createID(g), temp_coord, LEFT, '*', BULLET, NULL))
                        break;
                }
            }
            return -5;

        case SHOOT_UP:
            temp_coord.y--;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    if(!addObject(game, createID(g), temp_coord, UP, '*', BULLET, NULL))
                        break;
                }
            }
            return -5;

        case SHOOT_DOWN:
            temp_coord.y++;
            if (!checkWall(map_data, temp_coord)) {
                if (!checkCollision(game, temp_coord)) {
                    if(!addObject(game, createID(g), temp_coord, DOWN, '*', BULLET, NULL))
                        break;
                }
            }
            return -5;

        default:
            return -4;
    }

    // Object collided with players or bullets
    if (collision > 0) {

        memset(sendbuf, 0, MAXLINE);

        // Find the object that we collided with
        Gamestate* collided = findObject(game, collision);

        // Random coordinates for collision solving
        randomCoord(game, map_data, &temp_coord);

        // Player collided with a bullet
        if (g->type == PLAYER && collided->type == BULLET) {
            removeObject(game, collided->id);
            sprintf(sendbuf, "C%s ran into bullet!", g->name);
            sendAnnounce(game, sendbuf, strlen(sendbuf), 0);
            g->c = temp_coord;
            g->score = 0;
        }
        // Player collided with another player
        else if (g->type == PLAYER && collided->type == PLAYER)
            // Illegal move
            return -3;

        // Player collided with a score point 
        else if (g->type == PLAYER && collided->type == POINT) {
            g->c = collided->c;
            removeObject(game, collided->id);
            g->score++;
            // TODO: calculate points
            sprintf(sendbuf, "C%s has now %d points!", g->name, g->score);
            sendAnnounce(game, sendbuf, strlen(sendbuf), 0);
        }

        // Bullet collided with a player
        else if (g->type == BULLET && collided->type == PLAYER) {
            collided->c = temp_coord;
            sprintf(sendbuf, "C%s was hit by a bullet!", collided->name);
            sendAnnounce(game, sendbuf, strlen(sendbuf), 0);
            removeObject(game, g->id);
            collided->score = 0;
        }
        // Bullet collided with a point
        else if (g->type == BULLET && collided->type == POINT)
            removeObject(game, g->id);
        // Bullet collided with a bullet
        else if (g->type == BULLET && collided->type == BULLET) {
            removeObject(game, g->id);
            removeObject(game, collided->id);
        }

    }
    // Object collided with a wall
    else if (status > 0) {
        if (g->type == BULLET) {
            removeObject(game, g->id);
            //Return over 0 if list has changed
            return status;
        }
        // Illegal move for player
        return -3;
    }
    
    // No collision or collision solved
    //Return over 0 if list has changed
    return collision;
}

// Move every bullet once to direction that bullet is heading.
// This direction is determined by the Action enum in the bullet's data field
int updateBullets(Gamestate* g, Mapdata *map_data) {

    Gamestate* game = g;
    Gamestate* tmp;

    // If gamestate NULL
    if (!g)
        return -1;

    while (g != NULL) {
	tmp = g->next;
        if (g->type == BULLET) {
            // Bullets move as normal players
            if (processAction(game, map_data, g->id, g->data) > 0) {
		//If function returned over 0, list was changed
		g = tmp;
		continue;
	    }
        }
        g = g->next;
    }
    return 0;
}

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
    if ((status = addObject(g, createID(g), random, -1, '$', POINT, NULL))) {
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
int registerToMM(char *MM_IP, char *MM_port, char map_nr, struct sockaddr_storage* my_IP) {
    int sock;
    //struct sockaddr_in sock_addr_in;
    struct sockaddr_storage addr;
    //struct sockaddr_storage sock_addr_in;
    //struct sockaddr_storage sock_addr_in6;
    char message[2];
    socklen_t my_IP_len = sizeof(struct sockaddr_storage);
    int IP4, domain, so_reuseaddr = 1;

    // The message sent to MM server contains 'S' + map number
    message[0] = 'S';
    message[1] = map_nr;

    IP4 = isIpv4(MM_IP);

    //memset(&sock_addr_in6, 0, sizeof(struct sockaddr_storage));
    memset(&addr, 0, sizeof(struct sockaddr_storage));

    // Create socket of corred domain. Give it linger option because we will use this port again
    if(IP4) {
	domain = AF_INET;
    }
    else {
	domain = AF_INET6;
    }
    if ((sock = socket(domain, SOCK_STREAM, 0)) < 0) {
	perror("MM socket, create");
	return -1;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr)) == -1) {
	perror("setsockopt");
	exit(EXIT_FAILURE);
    }

    addr = ip_parser(MM_IP, MM_port);
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
	perror("MM socket, connect");
	return -1;
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
	if (getsockname(sock, (struct sockaddr *) my_IP, &my_IP_len) == -1) {
	    perror("getsockname");
	    return -1;
	} 
	/*if(IP4) {
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
	  }*/
        if(close(sock) == -1) {
	    perror("close");
	}
        return 0;
    }
    // Something went wrong
    else {
        if(close(sock) == -1) {
	    perror("close");
	}
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
ID checkCollision(Gamestate* g, Coord c) {

    // If Gamestate NULL
    if (!g)
        return -1;

    // Linked-list is empty
    if (g->next == NULL)
        return -2;

    // First element does not contain player
    g = g->next;

    while (g) {
        if (g->c.x == c.x && g->c.y == c.y) {
            return g->id;
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

