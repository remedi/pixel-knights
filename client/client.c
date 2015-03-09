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

#define LOCALGAME 1
#define NETGAME 2
#define BUFLEN 1000
#define MSGLEN 40

/*Global variables used by thread*/
//For chat service:
char **global_msg_arr;
int global_msg_count;
//Connection data:
int global_gametype;
int global_client_sock;
//Map data:
Mapdata global_map_data;
char **global_map = NULL;
//Own player data:
Playerdata global_my_player;
//Game data:
Gamedata global_game;


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
	printf("\nEnter max 40 bytes message: ");
	fgets(buf, 40, stdin);
	//Remove last newline:
	buf[strlen(buf)-1] = '\0';
	sprintf(buffer, "%s", buf);
	//new_message(fixed_buf, msg_array, msg_count);
    }
    printf("\n");
    return character;
}

//Check if anything occupies the given coordinates.
//Return 0 for nothing, 1 for wall, 2 for monster, 3 for player
char checkCoord(char **map, Gamedata gamedata, int x, int y) {
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
char *processCommand(char **map, Playerdata player, Gamedata gamedata, char input, char *buf) {
    char x = player.x_coord;
    char y = player.y_coord;
    /*    char init_x = x;
	  char init_y = y;*/
    char player_id = player.id;
    memset(buf, '\0', BUFLEN);
    switch(input) {
    case 'w':
	y--;
	break;
    case 's':
	y++;
	break;
    case 'a':
	x--;
	break;
    case 'd':
	x++;
	break;
    default:
	return buf;
	break;
    }
    if(checkCoord(map, gamedata, x, y) == 0) {
	sprintf(buf, "A%c%c%c", player_id, x, y);
	return buf;
    }
    //TODO: Combine these functions to a single function: checkCoord(). It returns 1 for wall, 2 for monster, 3 for player, 0 for empty.
    /*if(attackMonster(map, x, y) == 0) {
	sprintf(buf, "%c%c%c", player_id, init_x, init_y);
	return buf;
    }
    else if(checkWall(map, x, y) == 0) {
	sprintf(buf, "A%c%c%c", player_id, init_x, init_y);
	return buf;
    }
    else {
	sprintf(buf, "A%c%c%c", player_id, x, y);
	return buf;
	}*/

    return buf;
}

//Allocate memory for map. Initialize memory as map tiles. Return two-dimensional character array as the map.
char **createMap(Mapdata *map_data) {
    int height = map_data->height;
    int width = map_data->width;
    char **rows = map_data->map;
    char **old_ptr = rows;
    int i;

    rows = realloc(rows, sizeof(char **) * height);
    if(rows == NULL) {
	perror("drawMap, malloc");
	free(rows);
	free(old_ptr);
	return NULL;
    }

    //Allocate memory for each row and set initial tiles
    for(i = 0; i<height; i++) {
	rows[i] = malloc(sizeof(char *) * width);
	if(rows[i] == NULL) {
	    perror("drawMap, malloc");
	    free(rows);
	    return NULL;
	}
	memset(rows[i], ' ', width);
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
    buf++;
    players = realloc(players, game_data->player_count * sizeof(Playerdata));
    if(players == NULL) {
	free(players);
	free(old_ptr);
	perror("realloc");
	return NULL;
    }
    for(i = 0; i < game_data->player_count; i++) {
	players[i].id = *buf;
	buf++;
	players[i].x_coord = *buf;
	if(players[i].x_coord > width || players[i].x_coord < 1) {
	    printf("thread: G message failed sanity check. Player coordinates extend beyond map\n");
	    free(players);
	    return NULL;
	}
	buf++;
	players[i].y_coord = *buf;
	if(players[i].y_coord > height || players[i].y_coord < 1) {
	    printf("thread: G message failed sanity check. Player coordinates extend beyond map\n");
	    free(players);
	    return NULL;
	}
	buf++;
	players[i].sign = *buf;
	buf++;
	if(players[i].id == global_my_player.id) {
	    global_my_player.id = players[i].id;
	    global_my_player.x_coord = players[i].x_coord;
	    global_my_player.y_coord = players[i].y_coord;
	    global_my_player.sign = players[i].sign;
	}
    }
    return players;
}

//This function is called when A message is received. Update one players status from a character string received from the server.
void updateGame(char *buf, Gamedata *game_data) {
    buf++;
    Playerdata *players = game_data->players;
    int j = 0;
    int height = global_map_data.height;
    int width = global_map_data.width;
    while(*buf != '\0') {
	for(j = 0; j<(int) game_data->player_count; j++) {
	    if(players[j].id == *buf) {
		buf++;
		players[j].x_coord = *buf;
		if(players[j].x_coord > width || players[j].x_coord < 1) {
		    printf("thread: A message failed sanity check. Player coordinates extend beyond map\n");
		    return;
		}
		buf++;
		players[j].y_coord = *buf;
		if(players[j].y_coord > height || players[j].y_coord < 1) {
		    printf("thread: A message failed sanity check. Player coordinates extend beyond map\n");
		    return;
		}
		buf++;
		if(players[j].id == global_my_player.id) {
		    global_my_player.x_coord = players[j].x_coord;
		    global_my_player.y_coord = players[j].y_coord;
		}
	    }
	}
    }
    return;
}

//This function is a starting point for a thread. This thread is called when doing local testing. This function mimics server behaviour with it's messages.
void *local_server(void *arg) {
    int server_sock, client_sock;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_addr_len;
    char *buf = malloc(BUFLEN);
    char *id_msg = "I\005";
    char *game_msg = "G\001\005\001\001U";

    memset(buf, '\0', BUFLEN);

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
	else if(buf[0] == 'q') {
	    //Exit the thread:
	    printf("server: Sending local q message to thread\n");
	    write(client_sock, buf, 1);
	    break;
	}
	else {
	    printf("server: Forwarding message type: %c len: %lu to socket: %d\n", buf[0], strlen(buf), client_sock);
	    if(write(client_sock, buf, strlen(buf)) <= 0) {
		perror("server, write");
	    }
	}
    }

    free(buf);
    printf("server: Exiting\n");
    return 0;
}

//This function is a starting point for a thread. The point of this function is to read the messages received from server. It then calls the needed functions. Finally it adds players and monsters to the map array, draws the map to terminal and then removes characters and monsters from the map array.
void *updateMap(void *arg) {
    Playerdata *players = malloc(sizeof(Playerdata));
    Mapdata *map_data = &global_map_data;
    global_game.players = players;
    uint8_t x, y;
    int height = map_data->height;
    int i, j = 0;
    char *buf = malloc(BUFLEN);
    ssize_t bytes;
    struct sockaddr_un thread_addr;
    int sock_t = global_client_sock;

    strcpy(thread_addr.sun_path, "socket");
    thread_addr.sun_family = AF_UNIX;
    memset(buf, '\0', BUFLEN);

    printf("thread: Reading socket %d\n", sock_t);
    while(1) {
	//printf("Thread: Reading the pipe..\n");
	bytes = read(sock_t, buf, BUFLEN);
	if(bytes <= 0) {
	    //perror("thread, read");
	    continue;
	}
	else {
	    printf("thread: Read %lu bytes from a socket: %d\n", bytes, sock_t);
	}
	if(buf[0] == 'q') {
	    //Exit the thread:
	    printf("thread: Got local q message\n");
	    break;
	}
	else if(buf[0] == 'u') {
	    //Draw map again
	    printf("thread: Got local u message\n");
	}
	else if(buf[0] == 'C') {
	    printf("thread: Got C message\n");
	    buf++;
	    new_message(buf, global_msg_arr, global_msg_count);
	    buf--;
	}
	else if(buf[0] == 'A') {
	    if(strlen(buf) != 4) {
		printf("A message failed sanity check. Incorrect message length: %lu\n", strlen(buf));
		memset(buf, '\0', BUFLEN);
		continue;
	    }
	    printf("thread: Got A message\n");
	    updateGame(buf, &global_game);
	    memset(buf, '\0', BUFLEN);
	}
	else if(buf[0] == 'G') {
	    if((strlen(buf)-2)%4 != 0) {
		printf("G message failed sanity check. Incorrect message length: %lu\n", strlen(buf));
		memset(buf, '\0', BUFLEN);
		continue;
	    }
	    printf("thread: Got G message\n");
	    players = initGame(buf, &global_game, *map_data);
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

	//system("clear");
	for(i = 0; i<global_game.player_count; i++) {
	    //Add players
	    x = players[i].x_coord;
	    y = players[i].y_coord;
	    global_map[y][x] = players[i].sign;
	}
	printf("\n");
	for(i = 0; i<height; i++) {
	    //Actual drawing
	    printf("\t%s", global_map[i]);
	    printf("\t\t%s\n", global_msg_arr[global_msg_count-i-1]);
	}
	printf("\n");
	for(i = 0; i<global_game.player_count; i++) {
	    //Undo changes
	    x = players[i].x_coord;
	    y = players[i].y_coord;
	    global_map[y][x] = ' ';
	}

	printf("DEBUG: Drawing loop: %d\n", j);
	printf("HELP: Movement: \"wasd\" Quit: \"0\" or \"q\" Chat: \"c\"\n");
	j++;
	memset(buf, '\0', BUFLEN);
    }
    free(buf);
    free(players);
    printf("thread: Exiting\n");
    return 0;
}

int main(int argc, char *argv[]) {
    char input_char = 1;
    int i, height;
    struct sockaddr_un sock_addr;
    struct sockaddr_in sock_addr_in;
    char *buffer = malloc(BUFLEN);
    char *chat_buffer = malloc(BUFLEN);
    pthread_t thread, thread_server;
    ssize_t bytes;
    struct termios save_term, conf_term;
    char **message_array;
    int message_count;
    int new_term_settings;
    int sock;
    //struct stat buf;
    Mapdata map_data;

    if(argc == 1) {
	printf("-----------------\n");
	printf("No arguments given, defaulting to localgame\n");
	printf("For multiplayer game, use:\n");
	printf("client <server-ipv4> <server-port>\n");
	printf("-----------------\n");
	strcpy(global_my_player.name, "User");
	global_gametype = LOCALGAME;
    }
    else if(argc == 3) {
	printf("Joining a multiplayer game..\n");
	printf("Enter name: ");
	if(fgets(global_my_player.name, 10, stdin) == NULL) {
	    perror("fgets");
	}
	global_gametype = NETGAME;
    }
    else {
	printf("Unexpected amount of arguments given.\n");
	printf("For multiplayer, usage:\n");
	printf("client <ipv4> <port>\n");
	exit(-1);
    }

    //Delete previous socket, if the program was killed
    if(unlink("socket") == 0) {
	printf("main: Previous socket was found and removed.\n");
    }

    //Reserve memory for chat service
    message_count = 10;
    global_msg_count = message_count;
    message_array = malloc(sizeof(char *) * message_count);
    global_msg_arr = message_array;
    for(i = 0; i<message_count; i++) {
	message_array[i] = malloc(sizeof(char) * MSGLEN);
	memset(message_array[i], '\0', MSGLEN);
    }

    map_data.height = 10;
    map_data.width = 10;
    map_data.map = NULL;

    //Get terminal settings
    if(tcgetattr(STDIN_FILENO, &save_term) == -1) {
	perror("tcgetattr");
    }
    conf_term = save_term;
    new_term_settings = ICANON;
    conf_term.c_lflag = (conf_term.c_lflag & ~new_term_settings);
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
	if((conf_term.c_lflag & new_term_settings) == 0) {
	    printf("main: Terminal settings succesfully changed\n");
	}
	else {
	    printf("main: Error with term settings: Had: %d Wanted to add: %d. Bitwise: %d\n", conf_term.c_lflag, new_term_settings, (conf_term.c_lflag & new_term_settings));
	    exit(-1);
	}
    }


    //Create map
    global_map = createMap(&map_data);
    if(global_map == NULL) {
	printf("main: createMap error");
	exit(-1);
    }
    global_map_data = map_data;


    if(global_gametype == NETGAME) {
	printf("main: Initializing netgame\n");
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
	    perror("socket");
	}
	sock_addr_in = ipv4_parser(argv[1], argv[2]);
	sock_addr_in.sin_family = AF_INET;
	//Connect to the socket
	printf("main: Connecting to server..\n");
	if(connect(sock, (struct sockaddr *) &sock_addr_in, sizeof(sock_addr_in)) == -1) {
	    perror("main, connect");
	    printf("error number: %d\n", errno);
	    exit(-1);
	}
    }
    else if(global_gametype == LOCALGAME) {
	printf("main: Initializing localgame\n");
	if(pthread_create(&thread_server, NULL, local_server, NULL) < 0) {
	    perror("main, pthread_create");
	}
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock < 0) {
	    perror("main, socket");
	    exit(-1);
	}
	memset(&sock_addr, 0, sizeof(struct sockaddr_un));
	sock_addr.sun_family = AF_UNIX;
	strcpy(sock_addr.sun_path, "socket");
	//Loop until socket file is created by thread
	/*while(stat("socket", &buf) == -1) {
	  printf("waiting for socket.. ");
	  if(errno == 2) {
	  continue;
	  }
	  else {
	  perror("stat");
	  break;
	  }
	  }*/
	//Wait for socket. Use the loop (on top) or sleep (below)
	sleep(1);
	//Connect to the socket
	printf("main: Connecting to server..\n");
	if(connect(sock, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1) {
	    perror("main, connect");
	    printf("error number: %d\n", errno);
	    exit(-1);
	}
    }




    //Send hello message to server
    memset(buffer, '\0', BUFLEN);
    sprintf(buffer, "H%s", global_my_player.name);
    printf("main: Sending 'hello' message to the server: %s\n", buffer);
    if(write(sock, buffer, BUFLEN) < 1) {
	perror("main, write");
	exit(-1);
    }
    memset(buffer, '\0', BUFLEN);
    if(read(sock, buffer, 2) < 1) {
	perror("main, read");
    }
    if(buffer[0] == 'I') {
	//Receive ID message
	printf("main: Received my id from server\n");
	global_my_player.id = buffer[1];
    }
    else {
	printf("Unexpected response from server: %s\n", buffer);
    }

    //Start the thread
    global_client_sock = sock;
    if(pthread_create(&thread, NULL, updateMap, NULL) < 0) {
	perror("main, pthread_create");
    }

    while(1) {
	memset(buffer, '\0', BUFLEN);
	printf("main: Getting input..\n");
	//Get character or message from terminal
	input_char = getInput(buffer);
	if(input_char == 0) {
	    printf("main, getInput error\n");
	    exit(-1);
	}
	else if(input_char == 'c') {
	    memset(chat_buffer, '\0', BUFLEN);
	    sprintf(chat_buffer, "C%s: %s", global_my_player.name, buffer);
	    printf("main: Wrote ");
	    bytes = write(sock, chat_buffer, strlen(chat_buffer));
	    printf("%lu bytes to socket: %d\n", bytes, sock);
	    continue;
	}
	else if(input_char == 'q' || input_char == '0') {
	    break;
	}
	memset(buffer, '\0', BUFLEN);
	processCommand(global_map, global_my_player, global_game, input_char, buffer);
	if(strlen(buffer) == 0) {
	    printf("Not valid command!\n");
	    write(sock, "u", 1);
	    continue;
	}
	printf("main: Wrote ");
	bytes = write(sock, buffer, strlen(buffer));
	printf("%lu bytes to socket: %d\n", bytes, sock);
	//system("clear");
    }
  

    //Perform clean up:
    write(sock, "q", 1);
    if(pthread_join(thread, NULL) < 0) {
	perror("pthread_join");
    }
    if(pthread_join(thread_server, NULL) < 0) {
	perror("pthread_join");
    }
    //Free memory
    height = global_map_data.height;
    for(i = 0; i<height; i++) {
	free(global_map[i]);
    }
    free(global_map);
    free(buffer);
    free(chat_buffer);

    for(i = 0; i<message_count; i++) {
	free(message_array[i]);
    }

    free(message_array);

    unlink("socket");
  
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

    printf("main: Clean-up done, exiting\n");
    exit(0);
}
