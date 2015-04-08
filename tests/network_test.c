// This file includes network tests for server
//
// Authors: Pixel Knights
// Date:    8.4.2015

#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// This variable defines how many tests will be executed
#define TESTS 100

#define ADDR "127.0.0.1"
#define PORT 4375
#define MAXLINE 128

char buffer[MAXLINE];

// Test that the server responds to the P-message
bool testPoll(struct sockaddr_in* addr) {

    ssize_t n_bytes;
    int sock;

    // Create IPv4 TCP socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Clean the buffer
    memset(buffer, 0, MAXLINE);

    // Connect the socket
    if (connect(sock, (struct sockaddr*) addr, sizeof(struct sockaddr_in)) < 0) {
        perror("connect error");
        return false;
    }

    // Send Poll-message
    if (send(sock, "P", 1, 0) <= 0) {
        perror("send");
        return false;
    }

    if ((n_bytes = recv(sock, buffer, MAXLINE-1, 0)) <= 0) {
        perror("recv");
        return false;
    }

    if (n_bytes == 2)
        if (buffer[0] == 'R')
            return true;

    close(sock);

    return false;
}

int main(void) {

    struct sockaddr_in servaddr;

    // Fill the servaddr with server information ADDR and PORT
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ADDR, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton error");
        exit(EXIT_FAILURE);
    }

    printf("Testing Poll-message for %d times...\n", TESTS);
    for (int i = 0; i < TESTS; i++) {
        printf("Test %d/%d...\n", i+1, TESTS);
        if(!testPoll(&servaddr)) {
            printf("Test failed!\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("Test passed!\n");

    return 0;
}
