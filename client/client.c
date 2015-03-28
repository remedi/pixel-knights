#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "../maps/maps.h"
#include "client.h"
#include "update.h"

#define BUFLEN 1000
#define MSGLEN 40

//Mutex for preventing map updates when player is writing a chat message or reading help
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

//Parse ip and port from character strings to a struct sockaddr_in.
struct sockaddr_in ipv4_parser(char *ip, char *port) {
    struct sockaddr_in temp;
    memset(&temp, 0, sizeof(struct sockaddr_in));
    if(inet_pton(AF_INET, ip, &temp.sin_addr) < 1) {
        perror("ipv4_parser, inet_pton");
    }
    temp.sin_port = ntohs(strtol(port, NULL, 10));
    return temp;
}

//Read input character or chat message from terminal. If it's a chat message, store it in buffer.
char getInput(char *buffer) {
    char character;
    char buf[40];
    struct termios term_settings, old_settings;
    character = getchar();
    if(character == 'c') {
        pthread_mutex_lock(&mtx);
        printf("\nEnter max 40 bytes message: ");
	//Enable echo:
	if(tcgetattr(STDIN_FILENO, &term_settings) == -1) {
	    perror("tcgetattr");
	}
	memset(&old_settings, 0, sizeof(struct termios));
	old_settings = term_settings;
	term_settings.c_lflag = (term_settings.c_lflag | ECHO);
	term_settings.c_lflag = (term_settings.c_lflag | ICANON);
	if(tcsetattr(STDIN_FILENO, TCSANOW, &term_settings) == -1) {
	    perror("main, tcsetattr");
	}
        fgets(buf, 40, stdin);
	if(tcsetattr(STDIN_FILENO, TCSANOW, &old_settings) == -1) {
	    perror("main, tcsetattr");
	}
        //Remove last newline:
        sprintf(buffer, "%s", buf);
        pthread_mutex_unlock(&mtx);
    }
    return character;
}

//Process character read from terminal. Return a character string that is send to the server afterwads.
char *processCommand(char id, char input, char *buf) {
    Action a; 
    memset(buf, '\0', BUFLEN);
    switch(input) {
    case 'w':
	a = UP;
	break;
    case 's':
	a = DOWN;
	break;
    case 'a':
	a = LEFT;
	break;
    case 'd':
	a = RIGHT;
	break;
    default:
	return NULL;
    }
    sprintf(buf, "A%c", id);
    memcpy(buf+2, &a, 1);
    return buf;
}


int main(int argc, char *argv[]) {
    char input_char = 1;
    struct sockaddr_in sock_addr_in;
    char *buffer = malloc(BUFLEN);
    char *chat_buffer = malloc(BUFLEN);
    pthread_t thread;
    ssize_t bytes;
    char my_id, map_nr;
    char my_name[10];
    struct termios save_term, conf_term;
    int sock = 0;
    int exit_clean = 0;
    volatile sig_atomic_t thread_complete = 0;

    if(argc == 3 || argc == 2) {
        printf("Joining a multiplayer game..\n");
        printf("Enter name: ");
        if(fgets(my_name, 10, stdin) == NULL) {
            perror("fgets");
        }
	if(strlen(my_name) == 1) {
	    strcpy(my_name, "Default");
	}
	else {
	    //Remove last newline:
	    my_name[strlen(my_name)-1] = '\0';
	}
    }
    else {
        printf("Usage: client <ipv4> <port>\n");
        exit(EXIT_FAILURE);
    }

    //Get terminal settings
    if(tcgetattr(STDIN_FILENO, &save_term) == -1) {
        perror("tcgetattr");
    }
    conf_term = save_term;
    conf_term.c_lflag = (conf_term.c_lflag & ~ICANON);
    conf_term.c_lflag = (conf_term.c_lflag & ~ECHO);
    conf_term.c_lflag = (conf_term.c_lflag & ~ECHONL);
    //Set new settings
    if(tcsetattr(STDIN_FILENO, TCSANOW, &conf_term) == -1) {
        perror("main, tcsetattr");
    }
    //Check that mode actually changed
    memset(&conf_term, 0, sizeof(conf_term));
    if(tcgetattr(STDIN_FILENO, &conf_term) == -1) {
        perror("main, tcgetattr");
    }
    else {
        if((conf_term.c_lflag & (ICANON | ECHO | ECHONL)) == 0) {
            printf("main: Terminal settings succesfully changed\n");
        }
        else {
            perror("main: Error with term setattr");
            exit(EXIT_FAILURE);
        }
    }

    while(!exit_clean) {
	//Setup socket
	printf("main: Initializing netgame\n");
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
	    perror("socket");
	}
	memset(&sock_addr_in, 0, sizeof(struct sockaddr_in));
	if(argc == 3) {
	    sock_addr_in = ipv4_parser(argv[1], argv[2]);
	}
	else {
	    sock_addr_in = ipv4_parser("localhost", argv[1]);
	}
	sock_addr_in.sin_family = AF_INET;
	//Connect to the socket
	printf("main: Connecting to server..\n");
	if(connect(sock, (struct sockaddr *) &sock_addr_in, sizeof(sock_addr_in)) == -1) {
	    perror("main, connect");
	    printf("Exiting since connection failed\n");
	    exit_clean = 1;
	}
	if(!exit_clean) {
	    //Send hello message to server
	    memset(buffer, '\0', BUFLEN);
	    sprintf(buffer, "H%s", my_name);
	    printf("main: Sending 'hello' message to the server: %s\n", buffer);
	    if(write(sock, buffer, strlen(buffer) + 1) < 1) {
		perror("main, write");
		exit_clean = 1;
	    }
	    memset(buffer, '\0', BUFLEN);
	    if(read(sock, buffer, BUFLEN) < 1) {
		perror("main, read");
		exit_clean = 1;
	    }
	    if(buffer[0] == 'I') {
		//Receive ID message
		printf("main: ID message: ID: %d Map nr: %d\n", buffer[1], buffer[2]);
		my_id = buffer[1];
		map_nr = buffer[2];
		break;
	    }
	    else if(buffer[0] == 'L') {
		printf("main: Received serverlist, parsing first address\n");
		strtok(buffer, " ");
		//Use these pointers again. We are going another loop inside while.
		argv[1] = strtok(NULL, " ");
		argv[2] = strtok(NULL, " ");
		argc = 3;
		//redir_port = strtok(NULL, " ");
		//ipv4_parser(redir_ip, redir_port);
	    }
	    else if(buffer[0] == 'E') {
		printf("main: Connected to MM server but serverlist is empty\n");
		exit_clean = 1;
	    }
	    else {
		printf("Unexpected message from server: %s\n", buffer);
		//exit(EXIT_FAILURE);
		exit_clean = 1;
	    }
	}
    }

    // Fill the thread context struct and start the thread for updating map
    Contex_client_thread ctx;
    ctx.sock = &sock;
    ctx.lock = &mtx;
    ctx.done = &thread_complete; 
    ctx.map_nr = &map_nr;
    ctx.main_exit = &exit_clean;
    if(pthread_create(&thread, NULL, updateMap, &ctx) < 0) {
        perror("main, pthread_create");
    }

    while(!exit_clean) {
        memset(buffer, '\0', BUFLEN);
        //Get character or message from terminal
        input_char = getInput(buffer);
        if(!input_char) {
            printf("main, getInput error\n");
            exit(EXIT_FAILURE);
        }
        else if(input_char == 'c') {
            memset(chat_buffer, '\0', BUFLEN);
            sprintf(chat_buffer, "C%s: %s", my_name, buffer);
            printf("main: Wrote ");
            bytes = write(sock, chat_buffer, strlen(chat_buffer));
            printf("%lu bytes to socket: %d\n", bytes, sock);
            continue;
        }
        else if(input_char == 'q' || input_char == '0') {
            buffer[0] = 'Q';
            memcpy(buffer+1, &my_id, 1);
            write(sock, buffer, 2);
	    exit_clean = 1;
            break;
        }
        else if(input_char == 'h') {
            pthread_mutex_lock(&mtx);
            printf("HELP: Movement: \"wasd\" Quit: \"0\" or \"q\" Chat: \"c\"\n");
            printf("Press any key to continue\n");
            getchar();
            pthread_mutex_unlock(&mtx);
        }
        else {
            processCommand(my_id, input_char, buffer);
            if (!buffer) {
                printf("main: Unknown command\n");
                continue;
            }
            bytes = write(sock, buffer, 3);
        }

        // Thread sets this flag when completed
        if (thread_complete)
            break;
    }

    printf("\nmain: Commencing cleanup\n");

    //Perform cleanup:
    printf("main: Waiting for map update thread to exit\n");
    if(pthread_join(thread, NULL) < 0) {
        perror("pthread_join");
    }

    //Free memory allocated for chat messages
    free(buffer);
    free(chat_buffer);

    //Set back old settings
    tcsetattr(STDIN_FILENO, TCSANOW, &save_term);
    memset(&conf_term, 0, sizeof(conf_term));
    if(tcgetattr(STDIN_FILENO, &conf_term) == -1) {
        perror("tcgetattr");
    }
    if(conf_term.c_lflag == save_term.c_lflag) {
        printf("main: Old terminal settings succesfully restored\n");
    }
    else {
        printf("main: Error restoring terminal settings. Old: %d New: %d\n", save_term.c_lflag, conf_term.c_lflag);
    }

    printf("main: Cleanup done, exiting\n");
    exit(0);
}
