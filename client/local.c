#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/un.h>
#define BUFLEN 1000

// Frees memory pointed by 'ptr'. Used as thread cleanup handler
void free_memory(void *ptr) {
    printf("thread: Freeing my memory\n");
    free(ptr);
}

// This function is a starting point for a thread.
// This thread is called when doing local testing.
// This function mimics server behaviour with it's messages.
void *local_server(void *arg) {

    // Gets rid of the unused variable warning
    (void) arg;

    int server_sock, client_sock;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_addr_len;
    char *buf = malloc(BUFLEN);
    char *id_msg = "I\005";
    char *game_msg = "G\003\005\001\001U\006\002\002P\007\002\003R";

    memset(buf, '\0', BUFLEN);
    pthread_cleanup_push(free_memory, buf);

    memset(&client_addr_len, 0, sizeof(client_addr_len));
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(server_sock < 0) {
        perror("server, socket");
        exit(-1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, "socket");
    if(bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
    }
    if(listen(server_sock, 1) == -1) {
        perror("listen");
    }
    printf("server: Ready to accept a connection..\n");
    client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &client_addr_len);
    if(client_sock < 0) {
        perror("server, accept");
        exit(-1);
    }
    else {
        perror("server, accept");
    }

    while(1) {
        memset(buf, '\0', BUFLEN);
        printf("server: Reading socket %d..\n", client_sock);
        if(read(client_sock, buf, BUFLEN) <= 0) {
            perror("read");
            continue;
        }
        if(buf[0] == 'H') {
            printf("server: Responding to hello message\n");
            write(client_sock, id_msg, 2);
            write(client_sock, game_msg, strlen(game_msg));
        }
        else {
            printf("server: Forwarding message type: %c len: %lu to socket: %d\n", buf[0], strlen(buf), client_sock);
            if(write(client_sock, buf, strlen(buf)) <= 0) {
                perror("server, write");
            }
        }
    }

    pthread_cleanup_pop(1);
    printf("server: Exiting\n");
    return 0;
}
