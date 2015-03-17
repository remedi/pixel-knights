#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "client.h"
#include "local.h"

#define LOCALGAME 1
#define NETGAME 2
#define BUFLEN 1000
#define MSGLEN 40

#define MAP_HEIGHT 10
#define MAP_WIDTH 10
//Game data:
Gamedata global_game;
int glob_i = 0;
//Mutex for preventing map updates when player is writing a chat message or reading help
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

//Add the new message contained in buf to the msg_array. Also rotate pointers to make the newest message show as first.
char **new_message(char *buf, char **msg_array, int msg_count) {
    int i;
    char *temp_ptr = msg_array[msg_count-1];
    //Rearrange pointers
    for(i = (msg_count-1); i>0; i--) {
        msg_array[i] = msg_array[i-1];
    }
    msg_array[0] = temp_ptr;
    memset(msg_array[0], '\0', MSGLEN);
    //Add newest message as first
    memcpy(msg_array[0], buf, MSGLEN);
    return msg_array;
}

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
    character = getchar();
    if(character == 'c') {
	pthread_mutex_lock(&mtx);
        printf("\nEnter max 40 bytes message: ");
        fgets(buf, 40, stdin);
        //Remove last newline:
        sprintf(buffer, "%s", buf);
	pthread_mutex_unlock(&mtx);
    }
    //printf("\n");
    return character;
}

//Check if anything occupies the given coordinates.
//Return 0 for nothing, 1 for wall, 2 for monster, 3 for player
char checkCoord(char **map, Gamedata gamedata, int x, int y) {
    if(x < 0 || y < 0) {
        printf("Sanity check failed: Unaccepted pair of coordinates at checkCoord. x: %d y: %d\n", x, y);
        return -1;
    }
    int player_count = gamedata.player_count;
    int i;
    Playerdata *players = gamedata.players;
    //Check for walls
    if(map[y][x] == '#') {
        return 1;
    }
    //Check for players
    for(i = 0; i<player_count; i++) {
        if(players[i].x_coord == x && players[i].y_coord == y) {
            return 3;
        }
    }

    return 0;
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
            //F indicates 'Fail', not a valid command:
            buf[0] = 'F';
            return buf;
            break;
    }
    sprintf(buf, "A%c", id);
    memcpy(buf+2, &a, 1);
    return buf;
}

//Allocate memory for map. Initialize memory as map tiles. Return two-dimensional character array as the map.
char **createMap(Mapdata *map_data) {
    int height = map_data->height;
    int width = map_data->width;
    int i;

    // Allocate memory for map
    char **rows = malloc(sizeof(char *) * height);
    if(rows == NULL) {
        perror("drawMap, malloc");
        return NULL;
    }

    //Allocate memory for each row and set initial tiles
    for(i = 0; i<height; i++) {
        rows[i] = malloc(sizeof(char) * width + 1);
        if(rows[i] == NULL) {
            perror("drawMap, malloc");
            free(rows);
            return NULL;
        }
        // Write spaces and an ending zero to each line
        memset(rows[i], ' ', width-1);
        rows[i][width] = '\0';
    }

    //Add top and bottom wall
    memset(rows[0], '#', width);
    memset(rows[height-1], '#', width);

    //Add left and right walls
    for(i = 0; i<height; i++) {
        rows[i][0] = '#';
        rows[i][width-1] = '#';
    }

    return rows;
}

//This function is called when G message is received. Parse game status from a character string received from the server.
Playerdata *initGame(char *buf, Gamedata *game_data, Mapdata map_data) {
    buf++;
    int i;
    int height = map_data.height;
    int width = map_data.width;
    Playerdata *players = game_data->players;
    Playerdata *old_ptr = players;
    game_data->player_count = *buf;
    if(game_data->player_count < 1) {
        printf("Sanity check failed: player_count was: %d\n", game_data->player_count);
        free(players);
        return NULL;
    }
    buf++;
    players = realloc(players, game_data->player_count * sizeof(Playerdata));
    if(players == NULL) {
        //free(players);
        free(old_ptr);
        perror("realloc");
        return NULL;
    }
    for(i = 0; i < game_data->player_count; i++) {
        players[i].id = *buf;
        buf++;
        players[i].x_coord = *buf;
        if(players[i].x_coord > width || players[i].x_coord < 1) {
            printf("thread: G message failed sanity check. Player coordinates extend beyond map. X: %d\n", *buf);
            /*free(players);
              return NULL;*/
        }
        buf++;
        players[i].y_coord = *buf;
        if(players[i].y_coord > height || players[i].y_coord < 1) {
            printf("thread: G message failed sanity check. Player coordinates extend beyond map. Y: %d\n", *buf);
            /*free(players);
              return NULL;*/
        }
        buf++;
        players[i].sign = *buf;
        buf++;
    }
    return players;
}

//This function is a starting point for a thread. The point of this function is to read the messages received from server. It then calls the needed functions. Finally it adds players and monsters to the map array, draws the map to terminal and then removes characters and monsters from the map array.
void *updateMap(void *arg) {
    int *sock_t = arg;
    Playerdata *players = malloc(sizeof(Playerdata));
    Mapdata map_data;
    global_game.players = players;
    uint8_t x, y;
    int i, j = 0;
    ssize_t bytes;
    int break_flag = 1;

    //int sock_t = global_client_sock;
    char *buf = malloc(sizeof(char) * BUFLEN);

    char **rows;

    char **message_array;
    int message_count;

    // Initialize mapdata
    map_data.height = MAP_HEIGHT;
    map_data.width = MAP_WIDTH;
    rows = createMap(&map_data);
    if(rows == NULL) {
        printf("thread: createMap error");
        exit(-1);
    }


    //Reserve memory for chat service
    message_count = MAP_HEIGHT;
    message_array = malloc(sizeof(char *) * message_count);
    for(i = 0; i<message_count; i++) {
        message_array[i] = malloc(sizeof(char) * MSGLEN);
        memset(message_array[i], '\0', MSGLEN);
    }

    // Clean message buffer
    memset(buf, '\0', BUFLEN);

    //Cleanup handlers for thread cancellation
    pthread_cleanup_push(free_memory, buf);

    printf("thread: Reading socket %d\n", *sock_t);
    while(break_flag) {
        //This cleanup handler needs to be set again for each loop, in case the address of 'players' is changed.
        pthread_cleanup_push(free_memory, players);
        bytes = read(*sock_t, buf, BUFLEN);
        if(bytes < 0) {
            perror("read");
            continue;
        }
        else if(bytes == 0) {
            printf("thread: Server likely disconnected\n");
            break_flag = 0;
        }
        else if(buf[0] == 'C') {
            printf("thread: Got C message\n");
            buf++;
            new_message(buf, message_array, message_count);
            buf--;
        }
        else if(buf[0] == 'G') {
            if((bytes - 2) % 4 != 0) {
                printf("G message failed sanity check. Unaccepted message length: %lu\n", bytes);
                memset(buf, '\0', BUFLEN);
                continue;
            }
            players = initGame(buf, &global_game, map_data);
            if(players == NULL) {
                printf("thread: initGame failed\n");
                global_game.player_count = 0;
            }
            global_game.players = players;
            memset(buf, '\0', BUFLEN);
        }
        else {
            printf("thread: Received unkown message: %c. Ignoring\n", buf[0]);
            continue;
        }

        //Only draw map if user isn't busy doing something else
        if(pthread_mutex_trylock(&mtx) == 0) {
            //system("clear");
            for(i = 0; i<global_game.player_count; i++) {
                //Add players
                x = players[i].x_coord;
                y = players[i].y_coord;
                rows[y][x] = players[i].sign;
            }
            printf("\n");
            for(i = 0; i<map_data.height; i++) {
                //Actual drawing
                printf("\t%s", rows[i]);
                printf("\t\t%s\n", message_array[message_count-i-1]);
            }
            printf("\n");
            for(i = 0; i<global_game.player_count; i++) {
                //Undo changes
                x = players[i].x_coord;
                y = players[i].y_coord;
                rows[y][x] = ' ';
            }

            printf("DEBUG: Drawing loop: %d. Press 'h' for help\n", j);
            j++;
            pthread_mutex_unlock(&mtx);
        }
        memset(buf, '\0', BUFLEN);
        pthread_cleanup_pop(0);
    }
    // Free memory allocated for messages
    for(i = 0; i<message_count; i++) {
        free(message_array[i]);
    }
    free(message_array);

    //Remove cleanup handlers. Continue here if break_flag = 0
    for(i = 0; i<map_data.height; i++) {
        free(rows[i]);
    }
    free(rows);

    pthread_cleanup_pop(1);
    free(players);
    printf("thread: Read 0 bytes, exiting\n");
    return 0;
}

int main(int argc, char *argv[]) {
    char input_char = 1;
    struct sockaddr_in sock_addr_in;
    char *buffer = malloc(BUFLEN);
    char *chat_buffer = malloc(BUFLEN);
    pthread_t thread;
    ssize_t bytes;
    char my_id;
    char my_name[10];
    struct termios save_term, conf_term;
    int sock = 0;
    int exit_clean = 0;

    if(argc == 3) {
        printf("Joining a multiplayer game..\n");
        printf("Enter name: ");
        if(fgets(my_name, 10, stdin) == NULL) {
            perror("fgets");
        }
        //Remove last newline:
        my_name[strlen(my_name)-1] = '\0';
    }
    else {
        printf("Usage: client <ipv4> <port>\n");
        exit(-1);
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
            exit(-1);
        }
    }

    //Setup socket
    printf("main: Initializing netgame\n");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
	perror("socket");
    }
    memset(&sock_addr_in, 0, sizeof(struct sockaddr_in));
    sock_addr_in = ipv4_parser(argv[1], argv[2]);
    sock_addr_in.sin_family = AF_INET;
    //Connect to the socket
    printf("main: Connecting to server..\n");
    if(connect(sock, (struct sockaddr *) &sock_addr_in, sizeof(sock_addr_in)) == -1) {
	perror("main, connect");
	printf("error number: %d\n", errno);
	exit_clean = 1;
    }



    //Send hello message to server
    memset(buffer, '\0', BUFLEN);
    sprintf(buffer, "H%s", my_name);
    printf("main: Sending 'hello' message to the server: %s\n", buffer);
    if(write(sock, buffer, strlen(buffer) + 1) < 1) {
        perror("main, write");
        exit_clean = 1;
    }
    memset(buffer, '\0', BUFLEN);
    if(read(sock, buffer, 2) < 1) {
        perror("main, read");
    }
    if(buffer[0] == 'I') {
        //Receive ID message
        printf("main: Received my id: %d\n", buffer[1]);
        my_id = buffer[1];
    }
    else {
        printf("Unexpected message type from server: %c\n", buffer[0]);
        exit(-1);
    }

    //Start the thread
    global_client_sock = sock;
    if(pthread_create(&thread, NULL, updateMap, &sock) < 0) {
        perror("main, pthread_create");
    }

    while(!exit_clean) {
        memset(buffer, '\0', BUFLEN);
        //printf("main: Getting input..\n");
        //Get character or message from terminal
        input_char = getInput(buffer);
        if(input_char == 0) {
            printf("main, getInput error\n");
            exit(-1);
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
            break;
        }
        else if(input_char == 'z') {
            pthread_mutex_lock(&mtx);
	    printf("This info is currently disabled\n");
            printf("Press any key to continue\n");
            getchar();
            pthread_mutex_unlock(&mtx);
            continue;
        }
        else if(input_char == 'h') {
            pthread_mutex_lock(&mtx);
            printf("HELP: Movement: \"wasd\" Quit: \"0\" or \"q\" Chat: \"c\"\n");
            printf("DEBUG: Game info: \"z\"\n");
            printf("Press any key to continue\n");
            getchar();
            pthread_mutex_unlock(&mtx);
        }
        memset(buffer, '\0', BUFLEN);
        processCommand(my_id, input_char, buffer);
        if(buffer[0] == 'F') {
            //printf("Not valid command!\n");
            //write(sock, "u", 1);
            continue;
        }
        else if(strlen(buffer) == 0) {
            continue;
        }
        bytes = write(sock, buffer, strlen(buffer));
        //system("clear");
    }


    //Perform cleanup:
    printf("main: Canceling map update thread\n");
    if(pthread_cancel(thread) != 0) {
        perror("pthread_cancel");
    }
    printf("main: Waiting for map update thread to exit\n");
    if(pthread_join(thread, NULL) < 0) {
        perror("pthread_join");
    }
    //Free memory allocated for map


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
