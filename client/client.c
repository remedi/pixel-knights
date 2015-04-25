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
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "../maps/maps.h"
#include "../address.h"
#include "client.h"
#include "update.h"

#define BUFLEN 1000
#define MSGLEN 40
#define PORT_STR_LEN 6

//Global variable for exiting:
int exit_clean = 0;

//Mutex for preventing map updates when player is writing a chat message or reading help
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

//Parse list from from given buffer, that contains serverlist. 
//Present that list to user and ask which server they want to connect.
//Copy server choosed by user to the start of argument buffer
int serverListParser(char *buf) {

    // Server count is an ASCII number
    int server_count = buf[1] - '0';
    int i = 0, user_input = -1;
    char **server_array = malloc(sizeof(char*) * server_count);
    char *temp_server = malloc(BUFLEN);

    memset(temp_server, 0, BUFLEN);

    // Parse the server list
    server_array[i] = strtok(buf+3, "\200");
    for (i = 1; i < server_count; i++) {
        server_array[i] = strtok(NULL, "\200");
    }
    printf("\nServer list from MM server:\n");
    for (i = 0; i < server_count; i++) {
        printf("%d: %s\n", i, server_array[i]);
    }

    // Force the user to choose a valid character
    while (user_input < 0 || user_input >= server_count) {
        printf("\nChoose a valid server (0 - %d):\n>>", server_count-1);
        user_input = getchar() - '0';
    }

    strncpy(temp_server, server_array[user_input], strlen(server_array[user_input]));
    strncpy(buf, temp_server, strlen(temp_server));
    free(temp_server);

    free(server_array);
    return 0;
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
            perror("tcsetattr");
        }
        fgets(buf, 40, stdin);
        if(tcsetattr(STDIN_FILENO, TCSANOW, &old_settings) == -1) {
            perror("tcsetattr");
        }
        //Remove last newline:
        sprintf(buffer, "%s", buf);
        pthread_mutex_unlock(&mtx);
    }
    return character;
}

//Process character read from terminal. Return a character string that is send to the server afterwads.
char *processCommand(char id, unsigned char input, char *buf) {
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
	case 'i':
	    a = SHOOT_UP;
	    break;
	case 'k':
	    a = SHOOT_DOWN;
	    break;
	case 'j':
	    a = SHOOT_LEFT;
	    break;
	case 'l':
	    a = SHOOT_RIGHT;
	    break;
	default:
	    return NULL;
    }
    sprintf(buf, "A%c", id);
    memcpy(buf+2, &a, 1);
    return buf;
}

void clean_up() {
    printf("System signal received, commencing a clean exit\n");
    exit_clean = 1;
}

int changeTermSettings(struct termios new_settings) {
    //Set new settings
    struct termios actual_settings;
    if(tcsetattr(STDIN_FILENO, TCSANOW, &new_settings) == -1) {
	return -1;
    }
    //Check that mode actually changed
    memset(&actual_settings, 0, sizeof(struct termios));
    if(tcgetattr(STDIN_FILENO, &actual_settings) == -1) {
        return -1;
    }
    else {
        if(new_settings.c_lflag != actual_settings.c_lflag) {
	    return -1;
        }
        else {
            return 0;
        }
    }
}

int main(int argc, char *argv[]) {
    char input_char = 1;
    struct sockaddr_storage addr;
    struct sockaddr my_IP;
    char *buffer = malloc(BUFLEN);
    char *chat_buffer = malloc(BUFLEN);
    pthread_t thread;
    ssize_t bytes;
    char my_id, map_nr = 1, *ip, *port, IP_str[INET6_ADDRSTRLEN], port_str[PORT_STR_LEN];
    char my_name[10];
    memset(my_name, 0, 10);
    struct termios save_term, conf_term;
    int sock = 0, exit_msg_needed = 0, flags, udp_socket;
    socklen_t sock_len = sizeof(struct sockaddr_storage);
    socklen_t my_IP_len = sizeof(struct sockaddr);
    struct sigaction sig_hand;

    //Init strings
    memset(IP_str, 0, INET6_ADDRSTRLEN);
    memset(port_str, 0, PORT_STR_LEN);

    // Signal handler
    sig_hand.sa_handler = clean_up;
    sigemptyset(&sig_hand.sa_mask);
    sig_hand.sa_flags = 0;
    sigaction(SIGINT, &sig_hand, NULL);
    sigaction(SIGPIPE, &sig_hand, NULL);

    if (argc != 3 && argc != 2) {
        printf("Usage: client <ipv4> <port>\n");
	free(buffer);
	free(chat_buffer);
        exit(EXIT_FAILURE);
    }
    if (argc == 3) {
	addr = ip_parser(argv[1], argv[2]);
    }
    else {
        addr = ip_parser("localhost", argv[1]);
    }

    printf("Joining pixel knights multiplayer game...\n");
    // Force the player to input a valid name
    while (strlen(my_name) < 1 || my_name[0] == ' ') {
        printf("Enter name: ");
        if (fgets(my_name, 10, stdin) == NULL) {
            perror("fgets");
            exit_clean = 1;
	    break;
        }
        my_name[strlen(my_name)-1] = '\0';
    }

    //Get terminal settings
    if(tcgetattr(STDIN_FILENO, &save_term) == -1) {
        perror("tcgetattr");
	exit_clean = 1;
    }
    conf_term = save_term;
    conf_term.c_lflag = (conf_term.c_lflag & ~ICANON);
    conf_term.c_lflag = (conf_term.c_lflag & ~ECHO);
    //conf_term.c_lflag = (conf_term.c_lflag & ~ECHONL);

    if (changeTermSettings(conf_term) == -1) {
	perror("Error when changing terminal settings\n");
	exit_clean = 1;
    }


    //Setup connection. There maybe multiple loops if we first connect to a MM server
    while(!exit_clean) {
	if ((sock = socket(addr.ss_family, SOCK_STREAM, 0)) == 0) {
	    perror("socket");
	    exit_clean = 1;
	}
        // Connect to a server. It can be MM server or map server
        printf("main: Connecting to server..\n");
        if (connect(sock, (struct sockaddr *) &addr, sock_len) == -1) {
            perror("main, connect");
            printf("Exiting since connection failed\n");
            exit_clean = 1;
        }
        if (!exit_clean)  {
	    //Bind UDP socket here to receive gamestate packets from server
	    udp_socket = socket(addr.ss_family, SOCK_DGRAM, 0);
	    if(udp_socket == -1) {
		perror("UDP socket");
		exit_clean = 1;
	    }
	    //First get own address where to bind UDP socket
	    if(getsockname(sock, &my_IP, &my_IP_len) == -1) {
		perror("getsockname");
	    }
	    if(getnameinfo(&my_IP, my_IP_len, IP_str, INET6_ADDRSTRLEN, port_str, PORT_STR_LEN, NI_NUMERICSERV) != 0) {
		perror("getnameinfo");
	    }
	    printf("My ip: %s my port: %s\n", IP_str, port_str);
	    if(bind(udp_socket, &my_IP, my_IP_len) == -1) {
		perror("bind");
		exit_clean = 1;
	    }
            // Send hello message to server
            memset(buffer, '\0', BUFLEN);
            sprintf(buffer, "H%s%c %s %s", my_name, '\0', IP_str, port_str);
            if(write(sock, buffer, strlen(buffer) + 1) < 1) {
                perror("main, write");
                exit_clean = 1;
            }
            memset(buffer, '\0', BUFLEN);
            // Read message from socket. It starts with I if its map server, and L if its MM server
            if(read(sock, buffer, BUFLEN) < 1) {
                perror("main, read");
                exit_clean = 1;
            }
            if(buffer[0] == 'I') {
                //Receive ID message
                printf("main: ID message: ID: %d Map nr: %d\n", buffer[1], buffer[2]);
                my_id = buffer[1];
                map_nr = buffer[2];
		//Since we're connected to a map server, we need to send 'Q<ID>' when we exit the game
   	        exit_msg_needed = 1;
                // We have indentified to a map server so break here
		break;
	    }
	    else if(buffer[0] == 'L') {
		if(buffer[1] == '0') {
		    printf("main: Connected to MM server but serverlist is empty\n");
		    exit_clean = 1;
		    break;
		}
		if(serverListParser(buffer) != 0) {
		    printf("Error with serverListParser\n");
		    exit_clean = 1;
		    break;
		}
		ip = strtok(buffer, " ");
		port = strtok(NULL, " ");
		addr = ip_parser(ip, port);
	    }
	    else {
		printf("Unexpected message from server: %s\n", buffer);
		exit_clean = 1;
	    }
	}
    }

    // Fill the thread context struct and start the thread for updating map
    Context_client_thread ctx;
    ctx.sock = &sock;
    ctx.lock = &mtx;
    ctx.map_nr = &map_nr;
    ctx.main_exit = &exit_clean;
    if(pthread_create(&thread, NULL, updateMap, &ctx) < 0) {
	perror("main, pthread_create");
    }

    //This loop will constantly read user input
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
            printf("%zu bytes to socket: %d\n", bytes, sock);
            continue;
        }
        else if(input_char == 'q' || input_char == '0') {
            buffer[0] = 'Q';
            memcpy(buffer+1, &my_id, 1);
            write(sock, buffer, 2);
            exit_clean = 1;
	    exit_msg_needed = 0;
            break;
        }
        else if(input_char == 'h') {
            pthread_mutex_lock(&mtx);
            printf("HELP: Movement: 'wasd'\n");
            printf("HELP: Shooting: 'ijkl'\n");
            printf("HELP: Chatting: First press 'c', then type your message and press enter \n");
            printf("HELP: Quit: '0' or 'q'\n");
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

        // Thread can also set this flag when completed
        if (exit_clean)
            break;
    }
    
    //Clean-up:

    //Exit message flag
    if(exit_msg_needed) {
	//Make write non-blocking: We might be in a signal handler
        flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, (flags | O_NONBLOCK));
	buffer[0] = 'Q';
	memcpy(buffer+1, &my_id, 1);
	write(sock, buffer, 2);
    }

    if(pthread_join(thread, NULL) < 0) {
        perror("pthread_join");
    }

    //Free memory allocated for chat messages
    free(buffer);
    free(chat_buffer);

    //Set back old settings
    if(changeTermSettings(save_term) != 0) {
        printf("Error when restoring terminal settings\nIf you have somekind problems typing try one of the following:\n1. Just restart your terminal\n2. Press Ctrl + J, then type \"stty sane\" and press Ctrl + J\n");
    }

    printf("Game over, goodbye!\n");
    exit(0);
}
