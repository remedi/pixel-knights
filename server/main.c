// This is the Pixel Knights server
//
// Authors: Pixel Knights
// Date: 5.3.2015

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "../maps/maps.h"
#include "server.h"
#include "gamestate.h"
#include "thread.h"

#define MAXDATASIZE 128
#define PORT "4375"
#define BACKLOG 10

int main(int argc, char *argv[]) {

    // Declare variables
    struct addrinfo* results, hints, *i;
    struct sockaddr_storage their_addr;
    socklen_t socklen = sizeof(struct sockaddr);
    int status, listenfd, new_fd, fdmax, yes = 1;
    char map_nr = 1;
    fd_set rdset, master;
    char* recvbuf = malloc(MAXDATASIZE * sizeof(char));
    char* sendbuf = malloc(MAXDATASIZE * sizeof(char));
    char ipstr[INET_ADDRSTRLEN];
    ssize_t nbytes; 
    pthread_t gamestate_thread;
    Mapdata mapdata;

    // Initialize game state
    Gamestate game;
    memset(&game, 0, sizeof(Gamestate));
    ID id;
    Coord c;
    Action a;
    uint8_t* data;

    // Initialize hints struct for getaddrinfo
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //If player has defined map number (otherwise default is 1):
    if(argc == 2) {
	map_nr = strtol(argv[1], NULL, 10);
    }
    //Create map
    if(createMap(&mapdata, map_nr) != 0) {
	printf("Error creating map.\n");
	exit(EXIT_FAILURE);
    }
    //Announce this server to MM server, if user has given map_nr, ip and port
    if(argc == 4) {
	map_nr = strtol(argv[1], NULL, 10);
	if(connectMM(argv[2], argv[3], map_nr) != 0) {
	    printf("Error when announcing this server to MM server\n");
	}
    }

    // Try to get addrinfo, exiting on error
    if ((status = getaddrinfo(NULL, PORT, &hints, &results)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // Try to create socket and bind
    for (i = results; i != NULL; i = i->ai_next) {
        if ((listenfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol)) == -1) {
            perror("socket error");
            continue;
        }
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt error");
            exit(EXIT_FAILURE);
        }
        if (bind(listenfd, i->ai_addr, i->ai_addrlen) == -1) {
            perror("bind error");
            continue;
        }
        break;
    }

    if (i == NULL) {
        fprintf(stderr, "Could not find suitable address");
        exit(EXIT_FAILURE);
    }

    inet_ntop(i->ai_family, &((struct sockaddr_in*)i->ai_addr)->sin_addr, ipstr, INET_ADDRSTRLEN);

    // We don't need this anymore
    freeaddrinfo(results);

    // Listen for new connections
    if (listen(listenfd, BACKLOG) == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    // Set the maximum value of fd
    fdmax = listenfd;

    // Clear the sets
    FD_ZERO(&master);
    FD_ZERO(&rdset);

    // Set the listen socket to the master set
    FD_SET(listenfd, &master);

    printf("Server initialized!\nServer IP: %s\nServer port: %s\nWaiting for connections...\n", ipstr, PORT);

    // Initiate thread that keeps sending the clients the game state
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);
    Contex_server_thread ctx;
    ctx.g = &game;
    ctx.lock = &lock;
    if (pthread_create(&gamestate_thread, NULL, sendGamestate, &ctx) < 0) {
        perror("pthread_create error");
        exit(EXIT_FAILURE);
    }

    while(1) {

        // Copy the master set to read set
        rdset = master;

        // Select the descriptors that are ready for reading
        if ((status = select(fdmax + 1, &rdset, NULL, NULL, NULL)) == -1) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        // If the listen socket is ready for reading; new connection has arrived
        if (FD_ISSET(listenfd, &rdset)) {

            // Accept new connection and store their address to their_addr
            if ((new_fd = accept(listenfd, (struct sockaddr*)&their_addr, &socklen)) == -1) {
                perror("accept error");
                continue;
            }
            // Print the address from where the connection came
            inet_ntop(AF_INET, &((struct sockaddr_in*)&their_addr)->sin_addr,\
                      recvbuf, INET_ADDRSTRLEN);
            printf("New connection accepted from %s!\n", recvbuf);

            // Clear their_addr and socklen
            socklen = sizeof(struct sockaddr_storage);
            memset(&their_addr, 0, socklen);

            // Add the descriptor to master set and recalculate fdmax
            FD_SET(new_fd, &master);
            fdmax = max(new_fd, fdmax);
        }

        // Only the client sockets continue here
        else {
            FD_CLR(listenfd, &rdset);
            // Iterate every descriptor below fdmax
            for (int i = 0; i <= fdmax; i++) {

                // Clean the receive buffer
                memset(recvbuf, 0, MAXDATASIZE);

                // If i is ready for reading
                if (FD_ISSET(i, &rdset)) {

                    // Receive next message from the client
                    if ((nbytes = recv(i, recvbuf, MAXDATASIZE-1, 0)) == -1) {
                        perror("recv error");
                        continue;
                    }
                    recvbuf[nbytes] = '\0';

                    // If the message starts with A the message is an Action message
                    if (!strncmp(recvbuf, "A", 1)) {

                        // Action message must be of length 4 (A + ID + a)
                        if (nbytes != 3)
                            continue;

                        // Read ID and x,y coordinates from recvbuf
                        data = (uint8_t*) (recvbuf + 1);
                        id = *data;
                        a = *(data + 1);

                        // Move player and lock game state
                        pthread_mutex_lock(&lock);
                        status = movePlayer(&game, &mapdata, id, a);
                        pthread_mutex_unlock(&lock);

                        if (status == -1) {
                            fprintf(stderr, "movePlayer %02x: Invalid game state\n", id);
                            continue;
                        }
                        else if (status == -2) {
                            fprintf(stderr, "movePlayer %02x: ID not found\n", id);
                            continue;
                        }
                        else if (status == -3) {
                            fprintf(stderr, "movePlayer %02x: Illegal move\n", id);
                            continue;
                        }
                        else if (status == -4) {
                            fprintf(stderr, "movePlayer %02x: Action not found\n", id);
                            continue;
                        }
                    }

                    // If the message starts with H the message is a Hello-message
                    if (!strncmp(recvbuf, "H", 1)) {

                        // Hello message must be of length 2 or greater (H + username)
                        if (nbytes < 2)
                            continue;

                        id = createID();
                        c.x = 1;
                        c.y = 1;

                        // Add player and lock game state
                        pthread_mutex_lock(&lock);
                        status = addPlayer(&game, id, c, i, recvbuf[1]);
                        pthread_mutex_unlock(&lock);

                        if (status == -1) {
                            fprintf(stderr, "addPlayer %02x: Invalid game state\n", id);
                            continue;
                        }
                        else if (status == -2) {
                            fprintf(stderr, "addPlayer %02x: ID conflict\n", id);
                            continue;
                        }
                        sendbuf[0] = 'I';
                        memcpy(sendbuf + 1, &id, 1);
			memcpy(sendbuf + 2, &map_nr, 1);
                        if ((send(i, sendbuf, 3, 0)) == -1) {
                            perror("send error");
                            continue;
                        }

                        // Send announcement to all players about new player
                        char* message = malloc(sizeof(char)*(nbytes + 14));
                        recvbuf[nbytes] = '\0';
                        sprintf(message, "C%s joined game!", recvbuf+1);
                        status = sendAnnounce(&game, message, strlen(message), id);
                        free(message);
                        if (status == -1) {
                            fprintf(stderr, "sendAnnounce: Invalid game state\n");
                        }
                        else if (status == -2) {
                            fprintf(stderr, "sendAnnounce: Empty game\n");
                        }
                        else if (status == -3) {
                            fprintf(stderr, "sendAnnounce: Send error\n");
                        }
                        printf("Player with id: %02x connected!\n", id);
                    }

                    // If the message starts with C the message is a Chat-message
                    else if (!strncmp(recvbuf, "C", 1)) {
                        snprintf(sendbuf, nbytes, "%s", recvbuf);

                        // Broadcast the chat message to all sockets
                        for (int i = 0; i <= fdmax; i++) {
                            if (i == listenfd)
                                continue;
                            if (FD_ISSET(i, &master)) {
                                if ((nbytes = send(i, sendbuf, strlen(sendbuf)+1, 0)) == -1) {
                                    perror("send error");
                                    continue;
                                }
                            }
                        }
                    }

                    // If the message starts with Q the message is a Quit-message
                    else if (!strncmp(recvbuf, "Q", 1)) {

                        // Quit message must be of length 2 (Q + ID) 
                        if (nbytes != 2)
                            continue;

                        // Read ID from recvbuf and remove player
                        data = (uint8_t*) (recvbuf + 1);
                        id = *data;

                        // Remove player and lock game state
                        pthread_mutex_lock(&lock);
                        status = removePlayer(&game, id);
                        pthread_mutex_unlock(&lock);

                        if (status == -1) {
                            fprintf(stderr, "removePlayer %02x: Invalid game state\n", id);
                            continue;
                        }
                        else if (status == -2) {
                            fprintf(stderr, "removePlayer %02x: ID not found\n", id);
                            continue;
                        }
                        else
                            printf("Player with id: %02x disconnected!\n", id);

                        // Send disconnect announcement
                        status = sendAnnounce(&game, "CPlayer disconnected!", 21, 0); 
                        if (status == -1) {
                            fprintf(stderr, "sendAnnounce: Invalid game state\n");
                        }
                        else if (status == -2) {
                            fprintf(stderr, "sendAnnounce: Empty game\n");
                        }
                        else if (status == -3) {
                            fprintf(stderr, "sendAnnounce: Send error\n");
                        }

                        // Clear the descriptor from the master set
                        FD_CLR(i, &master);
                        close(i);
                        break;
                    }

                    // Just to be able to remotely close the server...
                    else if (!strncmp(recvbuf, "KILL", 4)) {
                        freePlayers(&game);
                        pthread_cancel(gamestate_thread);
                        pthread_join(gamestate_thread, NULL);
                        free(sendbuf);
                        free(recvbuf);
                        printf("Exiting...\n");
                        return 0;
                    }
                }
            }
        }
    }
}
