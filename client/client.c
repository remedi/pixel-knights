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

#include "client.h"

#define LOCALGAME 1
#define NETGAME 2
#define BUFLEN 1000
#define MSGLEN 40

/*Global variables used by thread*/
char **map = NULL;
struct gamedata game;
int sock;
int gametype;
char **msg_arr;
int global_msg_count;

char **new_message(char *buf, char **msg_array, int msg_count) {
  int i;
  char *temp_ptr = msg_array[msg_count-1];
  for(i = (msg_count-1); i>0; i--) {
    msg_array[i] = msg_array[i-1];
  }
  msg_array[0] = temp_ptr;
  memset(msg_array[0], '\0', BUFLEN);
  memcpy(msg_array[0], buf, BUFLEN);
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

char getInput(char **msg_array, int msg_count) {
  char character;
  char buf[40];
  character = getchar();
  if(character == 'c') {
    printf("\n Enter max 40 bytes message: ");
    fgets(buf, 40, stdin);
    //Remove last newline:
    buf[strlen(buf)-1] = '\0';
    new_message(buf, msg_array, msg_count);
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
    sprintf(buf, "%c%c%c", 1, init_x, init_y);
    return buf;
  }
  else if(checkWall(map, x, y) == 0) {
    sprintf(buf, "%c%c%c", 1, init_x, init_y);
    return buf;
  }
  else {
    //player->y_coord = y;
    //player->x_coord = x;
    sprintf(buf, "%c%c%c", 1, x, y);
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
  uint8_t player_count = 0;
  struct playerdata *players = game.players;
  int i = 0;
  memcpy(&player_count, buf, 1);
  buf++;

  //printf("Player count: %d\n", (int) player_count);
  while(*buf != '\0') {
    players[i].x_coord = *buf;
    buf++;
    players[i].y_coord = *buf;
    buf++;
    i++;
  }
  return;
}

//This function is handled by a thread
void *updateMap(void *arg) {
  struct playerdata *players = game.players;
  int player_count = game.player_count;
  int x, y;
  int height = game.map_height;
  int i, j = 0;
  char buf[BUFLEN];
  char prev_buf[BUFLEN];
  ssize_t bytes;
  memset(buf, '\0', BUFLEN);
  memset(prev_buf, '\0', BUFLEN);
  /*int pipe_read = open("cmd_pipe", 0);
  if(pipe_read < 0) {
    perror("open, thread");
    }*/
  if(map == NULL) {
    printf("updateMap received NULL as map\n");
    exit(-1);
  }
  while(1) {
    if(gametype == NETGAME) {
      printf("Thread: Waiting for server..\n");
      read(sock, buf, BUFLEN);
      printf("Thread: Server: %s\n", buf);
    }
    else if(gametype == LOCALGAME) {
      printf("Thread: Reading the pipe..\n");
      bytes = read(sock, buf, BUFLEN);
      if(bytes < 0) {
	perror("read, thread");
      }
      else {
	printf("Thread: Read %lu bytes from a pipe: %d\n", bytes, sock);
      }
      //Exit the thread:
      if(buf[0] == 'q') {
	break;
      }
      //Draw map again
      else if(buf[0] == 'u') {
	printf("Thread: Drawing map again\n");
	updateGame(prev_buf);
      }
      else {
	updateGame(buf);
	memset(prev_buf, '\0', BUFLEN);
	memcpy(prev_buf, buf, BUFLEN);
	memset(buf, '\0', BUFLEN);
      }
    }
    //system("clear");
    //Add players

    for(i = 0; i<player_count; i++) {
      x = players[i].x_coord;
      y = players[i].y_coord;
      map[y][x] = 'K';
    }
    //Add monsters
    //Actual drawing
    printf("\n");
    for(i = 0; i<height; i++) {
      printf("%s", map[i]);
      if(i<global_msg_count) {
	printf("\t\t%s\n", msg_arr[i]);
      }
      else {
	printf("\n");
      }
    }
    printf("\n");
    //Undo changes
    for(i = 0; i<player_count; i++) {
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
  int i;
  int height;
  char namebuffer[10];
  struct sockaddr_in addr;
  char *buffer = malloc(BUFLEN);
  pthread_t thread;
  ssize_t bytes;
  struct termios oldt, newt;
  char **message_array;
  int message_count;

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



  game.player_count = 1;
  game.monster_count = 0;
  game.map_height = 10;
  game.map_width = 10;

  message_count = game.map_height;
  global_msg_count = message_count;

  message_array = malloc(sizeof(char *) * message_count);
  msg_arr = message_array;

  for(i = 0; i<message_count; i++) {
    message_array[i] = malloc(sizeof(char) * BUFLEN);
    memset(message_array[i], '\0', BUFLEN);
  }


  player1.id = 1;
  player1.x_coord = 1;
  player1.y_coord = 1;
  player1.hp = 5;
  player1.sign = 'K';

  game.players = &player1;

  if(tcgetattr(STDIN_FILENO, &oldt) == -1) {
    perror("tcgetattr");
  }
  newt = oldt;
  newt.c_lflag = newt.c_lflag + ICANON;
  if(tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1) {
    perror("tcsetattr");
  }

  map = createMap(map, game);
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
      input_char = getInput(message_array, message_count);
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
      printf("Main: Writing to buffer\n");
      write(sock, buffer, BUFLEN);
    }
  }
  

  else if(gametype == LOCALGAME) {

    if(mkfifo("cmd_pipe", 00666) == -1) {
      perror("mkfifo");
    }
    sock = open("cmd_pipe", O_RDWR);
    if(sock < 0) {
      perror("open, main");
      exit(-1);
    }

    if(pthread_create(&thread, NULL, updateMap, NULL) < 0) {
      perror("pthread_create");
    }
    if(write(sock, "111", 1) < 1) {
      printf("Mapupdate error\n");
    }
    while(1) {
      input_char = getInput(message_array, message_count);
      if(input_char == 0) {
	printf("getInput error\n");
	exit(-1);
      }
      else if(input_char == 'c') {
	write(sock, "u", 1);
	continue;
      }
      else if(input_char == 'q' || input_char == '0') {
	break;
      }
      else if(input_char == 'p') {
	print_messages(message_array, message_count);
	write(sock, "u", 1);
	continue;
      }
      memset(buffer, '\0', BUFLEN);
      processCommand(map, &player1, input_char, buffer);
      if(strlen(buffer) == 0) {
	printf("Not valid command!\n");
	write(sock, "u", 1);
	continue;
      }
      printf("Main: Wrote ");
      bytes = write(sock, buffer, strlen(buffer));
      printf("%lu bytes to pipe: %d\n", bytes, sock);
      //system("clear");
    }
  }

  //Perform clean up:
  write(sock, "q", 1);
  if(pthread_join(thread, NULL) < 0) {
    perror("pthread_join");
  }
  //Free memory
  height = game.map_height;
  for(i = 0; i<height; i++) {
    free(map[i]);
  }
  free(map);
  free(buffer);
  unlink("cmd_pipe");
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  printf("Clean-up done, exiting\n");
  exit(0);
}
