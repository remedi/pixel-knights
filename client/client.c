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
char **global_map = NULL;
struct gamedata global_game;
int global_gametype;
char **global_msg_arr;
int global_msg_count;
struct sockaddr_in global_server_addr;
int global_client_sock;

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

struct sockaddr_in ipv4_parser(char *ip, char *port) {
  struct sockaddr_in temp;
  if(inet_pton(AF_INET, ip, &temp.sin_addr) < 1) {
    perror("ipv4_parser, inet_pton");
  }
  temp.sin_port = ntohs(strtol(port, NULL, 10));
  return temp;
}

char getInput(char *buffer) {
  char character;
  char buf[40];
  character = getchar();
  if(character == 'c') {
    printf("\n Enter max 40 bytes message: ");
    fgets(buf, 40, stdin);
    //Remove last newline:
    buf[strlen(buf)-1] = '\0';
    sprintf(buffer, "%s", buf);
    //new_message(fixed_buf, msg_array, msg_count);
  }
  printf("\n");
  return character;
}

int attackMonster(char **map, int x, int y) {
  //printf("No monster\n");
  return -1;
}

int checkWall(char **map, int x, int y) {
  //printf("DEBUG: Checking for wall at x: %d y: %d\n", x, y);
  if(map[y][x] == '#') {
    //printf("DEBUG: Wall detected\n");
    return 0;
  }
  else {
    return -1;
  }
}

char *processCommand(char **map, struct playerdata *player, char input, char *buf) {
  char x = player->x_coord;
  char y = player->y_coord;
  char init_x = x;
  char init_y = y;
  char player_id = player->id;
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

  //TODO: Combine these functions to a single function: checkCoord(). It returns 1 for wall, 2 for monster, 3 for player, 0 for empty.
  if(attackMonster(map, x, y) == 0) {
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
  }

  return buf;
}

char **createMap(char **rows, struct gamedata game) {
  int height = game.map_height;
  int width = game.map_width;
  int i;

  rows = realloc(rows, sizeof(char **) * height);
  if(rows == NULL) {
    perror("drawMap, malloc");
    return NULL;
  }

  //Allocate memory for each row and set initial tiles
  for(i = 0; i<height; i++) {
    rows[i] = malloc(sizeof(char *) * (width + 1));
    if(rows[i] == NULL) {
      perror("drawMap, malloc");
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
void updateGame(char *buf) {
  struct playerdata *players = global_game.players;
  int j = 0;
  buf++;
  while(*buf != '\0') {
    for(j = 0; j<(int) global_game.player_count; j++) {
      if(players[j].id == *buf) {
	buf++;
	players[j].x_coord = *buf;
	buf++;
	players[j].y_coord = *buf;
	buf++;
      }
    }
  }
  return;
}

void *local_server(void *arg) {
  int server_sock, client_sock;
  struct sockaddr_un server_addr, client_addr;
  socklen_t client_addr_len;
  char *buf = malloc(BUFLEN);
  char *id_msg = "I5";

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
  printf("server: Waiting for a connection..\n");
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
      write(client_sock, id_msg, 2);
    }
    else if(buf[0] == 'q') {
      //Exit the thread:
      printf("server: Exiting threads\n");
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
  return 0;
}

void *updateMap(void *arg) {
  //This function is handled by a thread
  struct playerdata *players = global_game.players;
  int player_count = global_game.player_count;
  int x, y;
  int height = global_game.map_height;
  int i, j = 0;
  char *buf = malloc(BUFLEN);
  ssize_t bytes;
  struct sockaddr_un thread_addr;
  int sock_t = global_client_sock;

  strcpy(thread_addr.sun_path, "socket");
  thread_addr.sun_family = AF_UNIX;
  memset(buf, '\0', BUFLEN);


  /*if(global_gametype == LOCALGAME) {
    sock_t = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sock_t < 0) {
      perror("thread, socket");
    }
    printf("Thread: Connecting to unix socket..\n");
    if(connect(sock_t, (struct sockaddr *) &thread_addr, sizeof(thread_addr)) < 0) {
      perror("thread, connect");
    }
    else {
      perror("Thread, connect");
    }
  }
  else if(global_gametype == NETGAME) {
    sock_t = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_t < 0) {
      perror("thread, socket");
    }
    printf("Thread: Connecting to unix socket..\n");
    if(connect(sock_t, (struct sockaddr *) &global_server_addr, sizeof(global_server_addr)) < 0) {
      perror("thread, connect");
    }
    else {
      perror("Thread, connect");
    }

  }
  else {
    printf("global_gametype not set!\n");
    exit(-1);
    }*/
  printf("thread: Reading socket %d\n", sock_t);
  while(1) {
    //printf("Thread: Reading the pipe..\n");
    bytes = read(sock_t, buf, BUFLEN);
    if(bytes <= 0) {
      //perror("thread, read");
      continue;
    }
    else {
      printf("Thread: Read %lu bytes from a socket: %d\n", bytes, sock_t);
    }
    if(buf[0] == 'q') {
      //Exit the thread:
      break;
    }
    else if(buf[0] == 'u') {
      //Draw map again
      printf("Thread: Drawing map based on old information\n");
    }
    else if(buf[0] == 'C') {
      buf++;
      new_message(buf, global_msg_arr, global_msg_count);
      buf--;
    }
    else if(buf[0] == 'A') {
      updateGame(buf);
      memset(buf, '\0', BUFLEN);
    }

    //system("clear");
    for(i = 0; i<player_count; i++) {
      //Add players
      x = players[i].x_coord;
      y = players[i].y_coord;
      global_map[y][x] = players[i].name[0];
    }
    //Add monsters
    printf("\n");
    for(i = 0; i<height; i++) {
      //Actual drawing
      printf("\t%s", global_map[i]);
      printf("\t\t%s\n", global_msg_arr[global_msg_count-i-1]);
    }
    printf("\n");
    for(i = 0; i<player_count; i++) {
      //Undo changes
      x = players[i].x_coord;
      y = players[i].y_coord;
      global_map[y][x] = ' ';
    }

    printf("HELP: Movement: \"wasd\" Quit: \"0\" or \"q\" Chat: \"c\"\n");
    printf("Drawing loop: %d\n", j);
    j++;
    memset(buf, '\0', BUFLEN);
    //sleep(1);
  }
  free(buf);
  printf("Thread exiting\n");
  return 0;
}

int main(int argc, char *argv[]) {
  char input_char = 1;
  struct playerdata player1;
  int i, height;
  char namebuffer[10];
  struct sockaddr_un sock_addr;
  char *buffer = malloc(BUFLEN);
  char *chat_buffer = malloc(BUFLEN);
  pthread_t thread, thread_server;
  ssize_t bytes;
  struct termios save_term, conf_term;
  char **message_array;
  int message_count;
  int new_term_settings;
  int sock;
  struct stat buf;

  if(argc == 1) {
    printf("-----------------\n");
    printf("No arguments given, defaulting to localgame\n");
    printf("For multiplayer game, use:\n");
    printf("client <server-ipv4> <server-port>\n");
    printf("-----------------\n");
    strcpy(namebuffer, "User");
    global_gametype = LOCALGAME;
  }
  else if(argc == 3) {
    printf("Joining a multiplayer game..\n");
    printf("Enter name: ");
    if(fgets(namebuffer, 10, stdin) == NULL) {
      perror("fgets");
    }
    global_gametype = NETGAME;
    global_server_addr = ipv4_parser(argv[1], argv[2]);
  }
  else {
    printf("Unknown amount of arguments given.\n");
    printf("For multiplayer, usage:\n");
    printf("client <ipv4> <port>\n");
    exit(-1);
  }

  //Delete previous socket, if the program was killed
  if(unlink("socket") == 0) {
    printf("main: Previous socket was found and removed.\n");
  }

  global_game.player_count = 1;
  global_game.monster_count = 0;
  global_game.map_height = 10;
  global_game.map_width = 10;

  message_count = global_game.map_height;
  global_msg_count = message_count;
  message_array = malloc(sizeof(char *) * message_count);
  global_msg_arr = message_array;

  for(i = 0; i<message_count; i++) {
    message_array[i] = malloc(sizeof(char) * MSGLEN);
    memset(message_array[i], '\0', MSGLEN);
  }

  player1.id = 1;
  player1.x_coord = 1;
  player1.y_coord = 1;
  player1.hp = 5;
  strcpy(player1.name, "User");

  global_game.players = &player1;

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

  global_map = createMap(global_map, global_game);
  if(global_map == NULL) {
    printf("createMap error");
    //exit(-1);
  }

  if(global_gametype == NETGAME) {
    printf("Initializing netgame\n");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    printf("Socket number: %d\n", sock);
    if(sock < 0) {
      perror("socket");
    }
    printf("Connecting to the server\n");
    if(connect(sock, (struct sockaddr *) &global_server_addr, sizeof(global_server_addr)) < 0) {
      perror("connect");
    }
    memset(buffer, '\0', BUFLEN);
    sprintf(buffer, "H%s", namebuffer);
    //Write name to server
    printf("Sending 'hello' message to the server: %s\n", buffer);
    if(write(sock, buffer, BUFLEN) < 1) {
      perror("write");
      exit(-1);
    }
    memset(buffer, '\0', BUFLEN);
    if(read(sock, buffer, BUFLEN) < 1) {
      perror("read");
    }
    if(buffer[0] == 'I') {
      player1.id = buffer[1];
    }
    else {
      printf("Unexpected response from server: %s\n", buffer);
    }
    //Start the thread
    pthread_create(&thread, NULL, updateMap, NULL);
    while(input_char != '0' && input_char != 'q') {
      memset(buffer, '\0', BUFLEN);
      input_char = getInput(buffer);
      if(input_char == 0) {
	printf("getInput error\n");
	exit(-1);
      }
      buffer = processCommand(global_map, &player1, input_char, buffer);
      if(strlen(buffer) == 0) {
	continue;
      }
      printf("Parent: Writing to buffer\n");
      write(sock, buffer, BUFLEN);
      memset(buffer, '\0', BUFLEN);
    }
  }

  else if(global_gametype == LOCALGAME) {
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
    while(stat("socket", &buf) == -1) {
      if(errno == 2) {
	continue;
      }
      else {
        perror("stat");
	break;
      }
    }
    //Connect to the socket
    if(connect(sock, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1) {
      perror("main, connect");
      printf("error number: %d\n", errno);
      exit(-1);
    }
    //Write name to server
    memset(buffer, '\0', BUFLEN);
    sprintf(buffer, "H%s", namebuffer);
    printf("main: Sending 'hello' message to the server: %s\n", buffer);
    if(write(sock, buffer, BUFLEN) < 1) {
      perror("main, write");
      exit(-1);
    }
    memset(buffer, '\0', BUFLEN);
    if(read(sock, buffer, BUFLEN) < 1) {
      perror("main, read");
    }
    if(buffer[0] == 'I') {
      printf("main: Got id message: %s\n", buffer);
      player1.id = buffer[1];
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
      //Get character or message from terminal
      memset(buffer, '\0', BUFLEN);
      printf("main: Getting input\n");
      input_char = getInput(buffer);
      if(input_char == 0) {
	printf("main, getInput error\n");
	exit(-1);
      }
      else if(input_char == 'c') {
	memset(chat_buffer, '\0', BUFLEN);
	sprintf(chat_buffer, "C%s: %s", player1.name, buffer);
	write(sock, chat_buffer, BUFLEN);
	continue;
      }
      else if(input_char == 'q' || input_char == '0') {
	break;
      }
      memset(buffer, '\0', BUFLEN);
      processCommand(global_map, &player1, input_char, buffer);
      if(strlen(buffer) == 0) {
	printf("Not valid command!\n");
	write(sock, "u", 1);
	continue;
      }
      printf("main: Wrote ");
      bytes = write(sock, buffer, strlen(buffer));
      //This message sometimes comes after the thread is updating map already, messing things up
      printf("%lu bytes to socket: %d\n", bytes, sock);
      //system("clear");
    }
  }

  //Perform clean up:
  write(sock, "q", 1);
  if(pthread_join(thread, NULL) < 0) {
    perror("pthread_join");
  }
  //Free memory
  height = global_game.map_height;
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
  
  //Set back oldsettings
  tcsetattr(STDIN_FILENO, TCSANOW, &save_term);
  memset(&conf_term, 0, sizeof(conf_term));
  if(tcgetattr(STDIN_FILENO, &conf_term) == -1) {
    perror("tcgetattr");
  }
  if(conf_term.c_lflag == save_term.c_lflag) {
    printf("Old terminal settings succesfully restored\n");
  }
  else {
    printf("Error restoring terminal settings. Old: %d New: %d\n", save_term.c_lflag, conf_term.c_lflag);
  }

  printf("Clean-up done, exiting\n");
  exit(0);
}
