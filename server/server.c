#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "server.h"


#define PORT "4375"
#define BACKLOG 10

int main(void) {

    struct addrinfo* results, hints, *i;
    struct sockaddr_storage their_addr;
    socklen_t socklen = sizeof(struct sockaddr);
    int status, sockfd, new_fd, yes = 1;

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

    // We don't need this anymore
    freeaddrinfo(results);

    // Listen for new connections
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for connections...\n");

    while(1) {

        // Execution blocks here until new connection is received
        if ((new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &socklen)) == -1) {
            perror("accept error");
            continue;
        }

        printf("New connection accepted!\n");
    }

    printf("Test hello\n");
    return 0;
}
