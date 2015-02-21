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
char **map = NULL;
struct gamedata global_game;
int sock;
int gametype;
char **msg_arr;
int global_msg_count;

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

void print_messages(char **msg_array, int msg_count) {
  int i;
  printf("Printing array\n");
  for(i = 0; i<msg_count; i++) {
    printf("%s", msg_array[i]);
  }
  printf("End of print\n");
}

struct sockaddr_in ipv4_parser(char *ip, char *port) {
  struct sockaddr_in temp;
  if(inet_pton(AF_INET, ip, &temp.sin_addr) < 1) {
    perror("ipv4_parser, inet_pton");
  }
  temp.sin_port = ntohs(strtol(port, NULL, 10));
  return temp;
}

char getInput(char **msg_array, int msg_count, char *prefix) {
  char character;
  char buf[40];
  char fixed_buf[50];
  character = getchar();
  if(character == 'c') {
    printf("\n Enter max 40 bytes message: ");
    fgets(buf, 40, stdin);
    //Remove last newline:
    buf[strlen(buf)-1] = '\0';
    sprintf(fixed_buf, "%s: %s", prefix, buf);
    new_message(fixed_buf, msg_array, msg_count);
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
  if(attackMonster(map, x, y) == 0) {
    sprintf(buf, "%c%c%c", player_id, init_x, init_y);
    return buf;
  }
  else if(checkWall(map, x, y) == 0) {
    sprintf(buf, "%c%c%c", player_id, init_x, init_y);
    return buf;
  }
  else {
    //player->y_coord = y;
    //player->x_coord = x;
    sprintf(buf, "%c%c%c", player_id, x, y);
    //printf("DEBUG: Returning command: %s\n", buf);
    return buf;
  }

  return buf;
}

char **createMap(char **rows, struct gamedata game) {
  int height = game.map_height;
  int width = game.map_width;
  int i;
  //unsigned int player_count = game.player_count;

  //printf("DEBUG: Generating the map.\n");
  //printf("DEBUG: Map height: %d map width: %d\n", game.map_height, game.map_width);
  //printf("DEBUG: Player count: %d, monster count: %d\n", player_count, game.monster_count);

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
  /*  uint8_t player_count = 0;
  memcpy(&player_count, buf, 1);
  buf++;*/

  //printf("Player count: %d\n", (int) player_count);
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

void *updateMap(void *arg) {
  //This function is handled by a thread
  struct playerdata *players = global_game.players;
  int player_count = global_game.player_count;
  int x, y;
  int height = global_game.map_height;
  int i, j = 0;
  char buf[BUFLEN];
  ssize_t bytes;
  struct sockaddr_un thread_addr;
  int sock_t;

  strcpy(thread_addr.sun_path, "socket");
  thread_addr.sun_family = AF_UNIX;
  memset(buf, '\0', BUFLEN);
  if(map == NULL) {
    printf("updateMap received NULL as map\n");
    exit(-1);
  }

  sock_t = socket(AF_UNIX, SOCK_STREAM, 0);
  if(sock_t < 0) {
    perror("thread, socket");
  }
  printf("Thread: Connecting..\n");
  if(connect(sock_t, (struct sockaddr *) &thread_addr, sizeof(thread_addr)) < 0) {
    perror("thread, connect");
  }
  else {
    perror("Thread, connect");
  }
  while(1) {
    if(gametype == NETGAME) {
      printf("Thread: Waiting for server..\n");
      read(sock_t, buf, BUFLEN);
      printf("Thread: Server: %s\n", buf);
    }
    else if(gametype == LOCALGAME) {
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
      else {
	updateGame(buf);
	memset(buf, '\0', BUFLEN);
      }
    }
    //system("clear");
    for(i = 0; i<player_count; i++) {
      //Add players
      x = players[i].x_coord;
      y = players[i].y_coord;
      map[y][x] = players[i].name[0];
    }
    //Add monsters
    printf("\n");
    for(i = 0; i<height; i++) {
      //Actual drawing
      printf("\t%s", map[i]);
      printf("\t\t%s\n", msg_arr[global_msg_count-i-1]);
    }
    printf("\n");
    for(i = 0; i<player_count; i++) {
      //Undo changes
      x = players[i].x_coord;
      y = players[i].y_coord;
      map[y][x] = ' ';
    }

    printf("HELP: Movement: \"wasd\" Quit: \"0\" or \"q\" Chat: \"c\"\n");
    printf("Drawing loop: %d\n", j);
    j++;
    memset(buf, '\0', BUFLEN);
    //sleep(1);
  }
  printf("Thread exiting\n");
  return 0;
}

int main(int argc, char *argv[]) {
  char input_char = 1;
  struct playerdata player1;
  int i, height, sockfd;
  char namebuffer[10];
  struct sockaddr_in addr;
  struct sockaddr_un sock_addr, other_end;
  char *buffer = malloc(BUFLEN);
  pthread_t thread;
  ssize_t bytes;
  struct termios save_term, conf_term;
  char **message_array;
  int message_count;
  socklen_t len_other_end;
  int new_term_settings;

  if(argc == 1) {
    printf("-----------------\n");
    printf("No arguments given, defaulting to localgame\n");
    printf("For multiplayer game, use:\n");
    printf("client <server-ipv4> <server-port>\n");
    printf("-----------------\n");
    gametype = LOCALGAME;
  }
  else if(argc == 3) {
    printf("Joining a multiplayer game..\n");
    printf("Enter name: ");
    if(fgets(namebuffer, 10, stdin) == NULL) {
      perror("fgets");
    }
    gametype = NETGAME;
    addr = ipv4_parser(argv[1], argv[2]);
  }
  else {
    printf("Unknown amount of arguments given.\n");
    printf("For multiplayer, usage:\n");
    printf("client <ipv4> <port>\n");
    exit(-1);
  }

  memset(&len_other_end, 0, sizeof(len_other_end));

  //Delete previous socket, if the program was killed
  if(unlink("socket") == 0) {
    printf("Previous socket was found and removed.\n");
  }

  global_game.player_count = 1;
  global_game.monster_count = 0;
  global_game.map_height = 10;
  global_game.map_width = 10;

  message_count = global_game.map_height;
  global_msg_count = message_count;

  message_array = malloc(sizeof(char *) * message_count);
  msg_arr = message_array;

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
    perror("tcsetattr");
  }
  //Check that mode actually changed
  memset(&conf_term, 0, sizeof(conf_term));
  if(tcgetattr(STDIN_FILENO, &conf_term) == -1) {
    perror("tcgetattr");
  }
  else {
    if((conf_term.c_lflag & new_term_settings) == 0) {
      printf("Terminal settings succesfully changed\n");
    }
    else {
      printf("Error with term settings: Had: %d Wanted to add: %d. Bitwise: %d\n", conf_term.c_lflag, new_term_settings, (conf_term.c_lflag & new_term_settings));
      exit(-1);
    }
  }

  map = createMap(map, global_game);
  if(map == NULL) {
    printf("createMap error");
    //exit(-1);
  }

  if(gametype == NETGAME) {
    printf("Initializing netgame\n");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    printf("Socket number: %d\n", sock);
    if(sock < 0) {
      perror("socket");
    }
    printf("Connecting to the server\n");
    if(connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
      perror("connect");
    }
    //Write name to server
    if(write(sock, namebuffer, 10) < strlen(namebuffer)) {
      printf("Connection succeeded but write failed\n");
      exit(-1);
    }

    while(input_char != '0' && input_char != 'q') {
      input_char = getInput(message_array, message_count, player1.name);
      if(input_char == 0) {
	printf("getInput error\n");
	exit(-1);
      }
      buffer = processCommand(map, &player1, input_char, buffer);
      if(strlen(buffer) == 0) {
	continue;
      }
      //read(sock, buffer, BUFLEN);
      //SEND COMMAND
      pthread_create(&thread, NULL, updateMap, NULL);
      printf("Parent: Writing to buffer\n");
      write(sock, buffer, BUFLEN);
    }
  }
  

  else if(gametype == LOCALGAME) {

    /*if(mkfifo("cmd_pipe", 00666) == -1) {
      perror("mkfifo");
      }
      sock = open("cmd_pipe", O_RDWR);*/

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sock < 0) {
      perror("Parent, socket");
      exit(-1);
    }
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sun_family = AF_UNIX;
    strcpy(sock_addr.sun_path, "socket");
    if(bind(sock, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1) {
      perror("bind");
    }
    if(pthread_create(&thread, NULL, updateMap, NULL) < 0) {
      perror("pthread_create");
    }
    if(listen(sock, 1) == -1) {
      perror("listen");
    }

    printf("Parent: Waiting for a connection..\n");
    sockfd = accept(sock, (struct sockaddr *) &other_end, &len_other_end);
    if(sockfd < 0) {
      perror("Parent, accept");
      exit(-1);
    }
    else {
      perror("Parent, accept");
    }
    printf("Parent: Sending update map command to the pipe %d\n", sockfd);
    if(write(sockfd, "u", 1) < 1) {
      printf("Mapupdate error\n");
    }
    while(1) {
      //Get character or message from terminal
      input_char = getInput(message_array, message_count, player1.name);
      if(input_char == 0) {
	printf("getInput error\n");
	exit(-1);
      }
      else if(input_char == 'c') {
	write(sockfd, "u", 1);
	continue;
      }
      else if(input_char == 'q' || input_char == '0') {
	break;
      }
      else if(input_char == 'p') {
	print_messages(message_array, message_count);
	write(sockfd, "u", 1);
	continue;
      }
      memset(buffer, '\0', BUFLEN);
      processCommand(map, &player1, input_char, buffer);
      if(strlen(buffer) == 0) {
	printf("Not valid command!\n");
	write(sockfd, "u", 1);
	continue;
      }
      printf("Parent: Wrote ");
      bytes = write(sockfd, buffer, strlen(buffer));
      //This message sometimes comes after the thread is updating map already, messing things up
      printf("%lu bytes to socket: %d\n", bytes, sockfd);
      //system("clear");
    }
  }

  //Perform clean up:
  write(sockfd, "q", 1);
  if(pthread_join(thread, NULL) < 0) {
    perror("pthread_join");
  }
  //Free memory
  height = global_game.map_height;
  for(i = 0; i<height; i++) {
    free(map[i]);
  }
  free(map);
  free(buffer);

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
