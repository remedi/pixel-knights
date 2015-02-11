#ifndef CLIENT_H
#define CLIENT_H

//Structs:
struct mapdata {
  unsigned int height;
  unsigned int width;
};

struct playerdata {
  unsigned int id;
  unsigned int x_coord;
  unsigned int y_coord;
  unsigned int hp;
  unsigned int sign;
};

struct gamedata {
  unsigned int player_count;
  unsigned int monster_count;
  struct playerdata *players;
};


//Functions:

//Draws the map to controlling terminal based on the current game state
int drawMap(struct mapdata, struct gamedata);

//Get action from terminal
char getInput();

//Handle received input
int processCommand(struct mapdata *, struct playerdata *, char);

//Read data from server
int readServer();

//Send data to server
int writeServer();

#endif
