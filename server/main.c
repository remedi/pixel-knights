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

#include "server.h"
#include "gamestate.h"

#define MAXDATASIZE 128
#define PORT "4375"
#define BACKLOG 10

int main(void) {

    // Declare variables
    struct addrinfo* results, hints, *i;
    struct sockaddr_storage their_addr;
    struct timeval tv;
    socklen_t socklen = sizeof(struct sockaddr);
    int status, listenfd, new_fd, fdmax, yes = 1;
    fd_set rdset, master;
    char recvbuf[MAXDATASIZE];
    char* sendbuf = malloc(MAXDATASIZE * sizeof(char));
    char ipstr[INET_ADDRSTRLEN];
    ssize_t nbytes; 

    // Initialize game state
    Gamestate game;
    memset(&game, 0, sizeof(Gamestate));

    // Initialize hints struct for getaddrinfo
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

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

    printf("Server initialized!\nServer IP: %s\nWaiting for connections...\n", ipstr);

    while(1) {

        // Copy the master set to read set
        rdset = master;

        // Re-initialize select timeout
        memset(&tv, 0, sizeof(struct timeval));
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        // Select the descriptors that are ready for reading
        if ((status = select(fdmax + 1, &rdset, NULL, NULL, &tv)) == -1) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        // Timeout happened
        else if (!status) {

            // Clear the send buffer
            memset(sendbuf, 0, MAXDATASIZE);

            // Parse game state message
            if ((status = parseGamestate(&game, sendbuf, MAXDATASIZE)) < 0)
                continue;

            // TODO: Make this in separate thread
            for (int i = 0; i <= fdmax; i++) {

                // Check that the fd is a client
                if (i == listenfd)
                    continue;
                if (FD_ISSET(i, &master)) {
                    // Send game state to every client
                    if ((nbytes = send(i, sendbuf, status, 0)) == -1) {
                        perror("send error");
                        continue;
                    }
                }
            }
        }

        // If the listen socket is ready for reading; new connection has arrived
        if (FD_ISSET(listenfd, &rdset)) {

            // Accept new connection and store their address to their_addr
            if ((new_fd = accept(listenfd, (struct sockaddr*)&their_addr, &socklen)) == -1) {
                perror("accept error");
                continue;
            }
            // TODO: At this point we can print the address from where the connection came
            printf("New connection accepted!\n");

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

                // If i is ready for reading
                if (FD_ISSET(i, &rdset)) {

                    // Receive next message from the client
                    if ((nbytes = recv(i, recvbuf, MAXDATASIZE-1, 0)) == -1) {
                        perror("recv error");
                        continue;
                    }

                    recvbuf[nbytes] = '\0';
                    printf("%s", recvbuf);

                    // If the message starts with H the message is a Hello-message
                    if (!strncmp(recvbuf, "H", 1)) {
                        ID id = createID();
                        Coord c;
                        c.x = 0;
                        c.y = 0;
                        addPlayer(&game, id, c, '#');
                        sprintf(sendbuf, "Hello %hhu\n", id);
                        if ((nbytes = send(i, sendbuf, strlen(sendbuf)+1, 0)) == -1) {
                            perror("send error");
                            continue;
                        }
                    }

                    // If the message starts with C the message is a Chat-message
                    else if (!strncmp(recvbuf, "C", 1)) {
                        snprintf(sendbuf, nbytes+7, "ECHO: %s\n", recvbuf);
                        if ((nbytes = send(i, sendbuf, strlen(sendbuf)+1, 0)) == -1) {
                            perror("send error");
                            continue;
                        }
                    }
                    // Quit
                    else if (!strncmp(recvbuf, "Q", 1)) {
                        if ((nbytes = send(i, "Bye!\n", 5, 0)) == -1) {
                            perror("send error");
                            continue;
                        }
                        // Clear the descriptor from the master set
                        FD_CLR(i, &master);
                        close(i);
                        break;
                    }
                    // Just to be able to remotely close the server...
                    else if (!strncmp(recvbuf, "KILL", 4)) {
                        free(sendbuf);
                        printf("Exiting...\n");
                        return 0;
                    }
                }
            }
        }
    }
}