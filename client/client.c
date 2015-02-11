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
  return character;
}

int processCommand(struct mapdata *map, struct playerdata *player, char input) {

  switch(input) {
  case 'w':
    player->y_coord = player->y_coord - 1;
    break;
  case 's':
    player->y_coord = player->y_coord + 1;
    break;
  case 'a':
    player->x_coord = player->x_coord - 1;
    break;
  case 'd':
    player->x_coord = player->x_coord + 1;
    break;
  default:
    return -1;
    break;
  }

  return 0;
}




int drawMap(struct mapdata map, struct gamedata game) {
  char **rows;
  unsigned int height = map.height + 2;
  unsigned int width = map.width + 2;
  int i;
  unsigned int player_count = game.player_count;
  struct playerdata *players = game.players;
  unsigned int x, y;

  printf("DEBUG: Drawing the map.\n");
  printf("DEBUG: Map height: %d map width: %d\n", map.height, map.width);
  printf("DEBUG: Player count: %d, monster count: %d\n", player_count, game.monster_count);

  rows = malloc(sizeof(char **) * height);
  if(rows == NULL) {
    perror("drawMap, malloc");
    return -1;
  }

  //Allocate memory for each row and set initial tiles
  for(i = 0; i<height; i++) {
    rows[i] = malloc(sizeof(char *) * (width + 1));
    if(rows[i] == NULL) {
      perror("drawMap, malloc");
      return -1;
    }
    memset(rows[i], '.', width);
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


  //Draw players
  for(i = 0; i<player_count; i++) {
    x = players[i].x_coord;
    y = players[i].y_coord;
    rows[y+1][x+1] = 'K';

  }

  //Set monsters

  //Actual drawing
  for(i = 0; i<height; i++) {
    printf("%s\n", rows[i]);
  }

  //Free memory
  for(i = 0; i<height; i++) {
    free(rows[i]);
  }
  free(rows);
  return 0;
}


int main(int argc, char *argv[]) {
  char input_char = 1;
  struct mapdata map;
  map.height = 10;
  map.width = 10;

  struct gamedata game;
  game.player_count = 1;
  game.monster_count = 0;

  struct playerdata player1;
  player1.id = 1;
  player1.x_coord = 0;
  player1.y_coord = 0;
  player1.hp = 5;
  player1.sign = 'K';

  game.players = &player1;

  if(drawMap(map, game) != 0) {
    printf("drawMap error");
    exit(-1);
  }
  printf("HELP: Movement: wasd Quit: 0 or q\n");
  printf("HELP: No collision detection: Segfault if u ran out of borders\n");

  while(input_char != '0' && input_char != 'q') {
    input_char = getInput();
    if(input_char == 0) {
      printf("getInput error\n");
      exit(-1);
    }
    processCommand(&map, &player1, input_char);
    system("clear");
    if(drawMap(map, game) != 0) {
      printf("drawMap error");
      exit(-1);
    }
    printf("HELP: Movement: wasd Quit: 0 or q\n");
    printf("HELP: No collision detection: Segfault if u ran out of borders\n");

  }


  printf("Drawing done, exiting\n");
  exit(0);
}
