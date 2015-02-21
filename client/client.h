#ifndef CLIENT_H
#define CLIENT_H

//Structs:

struct playerdata {
  char name[10];
  uint8_t id;
  uint8_t x_coord;
  uint8_t y_coord;
  uint8_t hp;
};

struct gamedata {
  uint8_t player_count;
  uint8_t monster_count;
  uint8_t map_height;
  uint8_t map_width;
  struct playerdata *players;
};


//Functions:

//Add new message to the chat service
char **new_message(char *, char **, int);

//For debugging, prints all of the messages stored in the chat service
void print_messages(char **, int);

//Create map by allocating memory and adding walls
char **createMap(char **, struct gamedata);

//Update map to show current player position and monster positions
//This is done by thread
void *updateMap(void *);

//Update game info from a string buffer
void updateGame(char *);

//Get character from terminal
char getInput(char **, int, char *);

//Handle received character
char *processCommand(char **, struct playerdata *, char, char *);

//Action related:
int attackMonster(char**, int, int);
int checkWall(char **, int, int);
int makeAMove(char **, int, int);

//NOT USED
//Read data from server
int readServer();

//Send data to server
int writeServer();

#endif
