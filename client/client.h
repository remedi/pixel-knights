#ifndef CLIENT_H
#define CLIENT_H

//Structs:

struct playerdata {
  uint8_t id;
  uint8_t x_coord;
  uint8_t y_coord;
  uint8_t hp;
  char sign;
};

struct gamedata {
  uint8_t player_count;
  uint8_t monster_count;
  uint8_t map_height;
  uint8_t map_width;
  struct playerdata *players;
};


//Functions:

char **new_message(char *, char **, int);

void print_messages(char **, int);

//Create map by allocating memory and drawing walls
char **createMap(char **, struct gamedata);

//Update map to show current player position and monster positions
//int updateMap(char **map, struct gamedata *);
void *updateMap(void *);

//Update game info from a string buffer
void updateGame(char *);

//Get action from terminal
char getInput(char **, int);

//Handle received input
char *processCommand(char **, struct playerdata *, char, char *);

//Action related:
int attackMonster(char**, int, int);
int checkWall(char **, int, int);
int makeAMove(char **, int, int);

//Read data from server
int readServer();

//Send data to server
int writeServer();

#endif
