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

//Return 1 for IPv4 and 0 for all the others (We expect IPv6 then)
int isIpv4(char *buf) {
    int len = strlen(buf);
    int dot_count = 0, colon_count = 0, i;
    for(i = 0; i < len; i++) {
	if(buf[i] == '.')
	    dot_count++;
	if(buf[i] == ':')
	    colon_count++;
    }
    //Simple sanity check:
    if(dot_count == 0 && colon_count == 0)
	return -1;

    if(dot_count > colon_count)
	return 1;
    else
	return 0;
}


//Parse list from from given buffer, that contains serverlist. 
//Present that list to user and ask which server they want to connect.
//Copy server choosed by user to the start of argument buffer
int serverListParser(char *buf) {

    // Server count is an ASCII number
    int server_count = buf[1] - '0';
    int i = 0, user_input = -1;
    char **server_array = malloc(sizeof(char*) * server_count);

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

    strncpy(buf, server_array[user_input], strlen(server_array[user_input]));

    free(server_array);
    return 0;
}

//Parse ip and port from character strings to a struct sockaddr_in6.
struct sockaddr_in6 ipv6_parser(char *ip, char *port) {
  struct sockaddr_in6 temp;
  if(inet_pton(AF_INET6, ip, &temp.sin6_addr) < 1) {
    perror("ipv6_parser, inet_pton");
    exit(-1);
  }
  temp.sin6_port = ntohs(strtol(port, NULL, 10));
  temp.sin6_family = AF_INET6;
  return temp;
}

//Parse ip and port from character strings to a struct sockaddr_in.
struct sockaddr_in ipv4_parser(char *ip, char *port) {
    struct sockaddr_in temp; 
    memset(&temp, 0, sizeof(struct sockaddr_in));
    if(inet_pton(AF_INET, ip, &temp.sin_addr) < 1) {
        perror("ipv4_parser, inet_pton");
    }
    temp.sin_port = ntohs(strtol(port, NULL, 10));
    temp.sin_family = AF_INET;
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
    struct sockaddr_in6 sock_addr_in6;
    struct sockaddr *addr_ptr;
    char *buffer = malloc(BUFLEN);
    char *chat_buffer = malloc(BUFLEN);
    pthread_t thread;
    ssize_t bytes;
    char my_id, map_nr, *ip, *port;
    char my_name[10];
    memset(my_name, 0, 10);
    struct termios save_term, conf_term;
    int sock = 0;
    int exit_clean = 0;
    int isIp4;
    socklen_t sock_len;

    if (argc != 3 && argc != 2) {
        printf("Usage: client <ipv4> <port>\n");
        exit(EXIT_FAILURE);
    }
    if (argc == 3) {
	isIp4 = isIpv4(argv[1]);
	if(isIp4) {
	    sock_addr_in = ipv4_parser(argv[1], argv[2]);
	    sock_len = sizeof(sock_addr_in);
	    addr_ptr = (struct sockaddr *) &sock_addr_in;
	}
	else {
	    sock_addr_in6 = ipv6_parser(argv[1], argv[2]);
	    sock_len = sizeof(sock_addr_in6);
	    addr_ptr = (struct sockaddr *) &sock_addr_in6;
	}
    }
    else {
        sock_addr_in = ipv4_parser("localhost", argv[1]);
	addr_ptr = (struct sockaddr *) &sock_addr_in;
    }

    printf("Joining pixel knights multiplayer game...\n");
    // Force the player to input a valid name
    while (strlen(my_name) < 1 || my_name[0] == ' ') {
        printf("Enter name: ");
        if (fgets(my_name, 10, stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        my_name[strlen(my_name)-1] = '\0';
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
            //printf("main: Terminal settings succesfully changed\n");
        }
        else {
            perror("main: Error with term setattr");
            exit(EXIT_FAILURE);
        }
    }

    //Setup connection. There maybe multiple loops if we first connect to a MM server
    while(!exit_clean) {
	if(isIp4) {
	    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket, IPv4");
		exit_clean = 1;
	    }
	}
	else {
	    if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) == 0) {
		perror("socket, IPv6");
		exit_clean = 1;
	    }
	}
        // Connect to a server. It can be MM server or map server
        printf("main: Connecting to server..\n");
        if (connect(sock, addr_ptr, sock_len) == -1) {
            perror("main, connect");
            printf("Exiting since connection failed\n");
            exit_clean = 1;
        }
        if (!exit_clean) {
            // Send hello message to server
            memset(buffer, '\0', BUFLEN);
            sprintf(buffer, "H%s", my_name);
            //printf("main: Sending 'hello' message to the server: %s\n", buffer);
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
		if(isIpv4(ip)) {
		    sock_addr_in = ipv4_parser(ip, port);
		    sock_len = sizeof(sock_addr_in);
		    addr_ptr = (struct sockaddr *) &sock_addr_in;
		}
		else {
		    sock_addr_in6 = ipv6_parser(ip, port);
		    sock_len = sizeof(sock_addr_in6);
		    addr_ptr = (struct sockaddr *) &sock_addr_in6;
		}
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

        // Thread can also set this flag when completed
        if (exit_clean)
            break;
    }

    //printf("\nmain: Commencing cleanup\n");

    //Perform cleanup:
    //printf("main: Waiting for map update thread to exit\n");
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
        //printf("main: Old terminal settings succesfully restored\n");
    }
    else {
        printf("main: Error restoring terminal settings. Old: %d New: %d\n", save_term.c_lflag, conf_term.c_lflag);
    }

    //printf("main: Cleanup done, exiting\n");
    printf("Game over, goodbye!\n");
    exit(0);
}
