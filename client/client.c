#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "client.h"

char getInput() {
  char character;
  if(system("stty raw") == -1) {
    perror("system, raw");
    return 0;
  }
  character = getchar();
  if(system("stty cooked") == -1) {
    perror("system, cooked");
    return 0;
  }
  printf("\n");
  return character;
}

int attackMonster(char **map, int x, int y) {
  printf("No monster\n");
  return -1;
}

int checkWall(char **map, int x, int y) {
  printf("DEBUG: Checking for wall at x: %d y: %d\n", x, y);
  if(map[y][x] == '#') {
    printf("DEBUG: Wall detected\n");
    return 0;
  }
  else {
    return -1;
  }
}

int processCommand(char **map, struct playerdata *player, char input) {
  int x = player->x_coord;
  int y = player->y_coord;

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
    return -1;
    break;
  }
  if(attackMonster(map, x, y) == 0) {
    return 0;
  }
  else if(checkWall(map, x, y) == 0) {
    return 0;
  }
  else {
    player->y_coord = y;
    player->x_coord = x;
    return 0;
  }

  return -1;
}




char **createMap(char **rows, struct gamedata game) {

  int height = game.map_height;
  int width = game.map_width;
  int i;
  unsigned int player_count = game.player_count;

  printf("DEBUG: Generating the map.\n");
  printf("DEBUG: Map height: %d map width: %d\n", game.map_height, game.map_width);
  printf("DEBUG: Player count: %d, monster count: %d\n", player_count, game.monster_count);

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

int updateMap(char **map, struct gamedata game) {
  struct playerdata *players = game.players;
  int player_count = game.player_count;
  int x, y;
  int height = game.map_height;
  int i;
  if(map == NULL) {
    printf("updateMap received NULL as map\n");
    return -1;
  }

  //Draw players
  for(i = 0; i<player_count; i++) {
    x = players[i].x_coord;
    y = players[i].y_coord;
    map[y][x] = 'K';
  }

  //Draw monsters

  //Actual drawing
  for(i = 0; i<height; i++) {
    printf("%s\n", map[i]);
  }

  //Undo changes
  for(i = 0; i<player_count; i++) {
    x = players[i].x_coord;
    y = players[i].y_coord;
    map[y][x] = ' ';
  }

  return 0;
}

int main(int argc, char *argv[]) {
  char input_char = 1;
  char **map = NULL;
  int i;
  int height;

  struct gamedata game;
  game.player_count = 1;
  game.monster_count = 0;
  game.map_height = 10;
  game.map_width = 10;

  struct playerdata player1;
  player1.id = 1;
  player1.x_coord = 1;
  player1.y_coord = 1;
  player1.hp = 5;
  player1.sign = 'K';

  game.players = &player1;

  map = createMap(map, game);
  if(map == NULL) {
    printf("createMap error");
    exit(-1);
  }
  if(updateMap(map, game) != 0) {
    printf("updateMap error");
    exit(-1);
  }
  while(input_char != '0' && input_char != 'q') {
    input_char = getInput();
    if(input_char == 0) {
      printf("getInput error\n");
      exit(-1);
    }
    processCommand(map, &player1, input_char);
    //system("clear");
    if(updateMap(map, game) != 0) {
      printf("drawMap error");
      exit(-1);
    }
    printf("HELP: Movement: wasd Quit: 0 or q\n");
  }


  //Free memory
  height = game.map_height;
  for(i = 0; i<height; i++) {
    free(map[i]);
  }
  free(map);

  printf("Drawing done, exiting\n");
  exit(0);
}
