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

#define MAXDATASIZE 128
#define PORT "4375"
#define BACKLOG 10

int main(void) {

    struct addrinfo* results, hints, *i;
    struct sockaddr_storage their_addr;
    socklen_t socklen = sizeof(struct sockaddr);
    int status, sockfd, new_fd, yes = 1;
    char recvbuf[MAXDATASIZE];
    char sendbuf[MAXDATASIZE];
    char ipstr[INET_ADDRSTRLEN];
    ssize_t nbytes; 

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
        if ((sockfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol)) == -1) {
            perror("socket error");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt error");
            exit(EXIT_FAILURE);
        }
        if (bind(sockfd, i->ai_addr, i->ai_addrlen) == -1) {
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
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    printf("Server initialized!\nServer IP: %s\nWaiting for connections...\n", ipstr);

    while(1) {

        // Execution blocks here until new connection is received
        if ((new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &socklen)) == -1) {
            perror("accept error");
            continue;
        }

        // Receive next message from the client
        if ((nbytes = recv(new_fd, recvbuf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv error");
            close(new_fd);
            continue;
        }
        recvbuf[nbytes] = '\0';
        printf("%s", recvbuf);

        // If the message starts with H the message is a Hello-message
        if (!strncmp(recvbuf, "H", 1)) {
            if ((nbytes = send(new_fd, "Hello!\n", 7, 0)) == -1) {
                perror("send error");
                close(new_fd);
                continue;
            }
        }
        
        // If the message starts with C the message is a Chat-message
        else if (!strncmp(recvbuf, "C", 1)) {
            snprintf(sendbuf, nbytes+6, "ECHO: %s", recvbuf);
            if ((nbytes = send(new_fd, sendbuf, nbytes+6, 0)) == -1) {
                perror("send error");
                close(new_fd);
                continue;
            }
        }
        else if (!strncmp(recvbuf, "Q", 1)) {
            if ((nbytes = send(new_fd, "Bye!\n", 5, 0)) == -1) {
                perror("send error");
                close(new_fd);
                continue;
            }
            close(new_fd);
            break;
        }

        close(new_fd);
    }

    printf("Exiting...\n");
    return 0;
}
