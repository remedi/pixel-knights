#ifndef CLIENT_H
#define CLIENT_H

//Structs:

typedef struct playerdata {
  char name[10];
  uint8_t id;
  uint8_t x_coord;
  uint8_t y_coord;
  uint8_t hp;
  uint8_t sign;
}Playerdata;

typedef struct gamedata {
  uint8_t player_count;
  uint8_t monster_count;
  Playerdata *players;
}Gamedata;

typedef struct mapdata {
  uint8_t height;
  uint8_t width;
  char **map;
}Mapdata;

//Functions:

//Local server, for testing purposes
void *local_server(void *);

//Add new message to the chat service
char **new_message(char *, char **, int);

//Create map by allocating memory and adding walls
//char **createMap(char **, struct gamedata);
char **createMap(struct mapdata *);

//Update map to show current player position and monster positions
//This is done by thread
void *updateMap(void *);

//For handling the 'G' message: create struct that contains info of all of the players and monsters
struct playerdata *initGame(char *, Gamedata *, Mapdata);

//For handling the 'A' message: A single action
void updateGame(char *, Gamedata *);

//Get character from terminal
char getInput(char *);

//Handle received character
char *processCommand(char **, Playerdata, Gamedata, char, char *);

//Check coordinates. Return 0 for empty, 1 for wall, 2 for monster, 3 for another player
char checkCoord(char **, Gamedata, int, int);

#endif
