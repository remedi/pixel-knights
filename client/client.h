#ifndef CLIENT_H
#define CLIENT_H

#include <termios.h>
#include <unistd.h>

#include "../typedefs.h"

// Functions:

//Parse list from from given buffer, that contains serverlist. 
//Present that list to user and ask which server they want to connect.
//Copy server choosed by user to the start of argument buffer
int serverListParser(char *);

//Signal handler
void clean_up();

// Get character from terminal
char getInput(char *);

// Handle received character
char *processCommand(char, unsigned char, char *);

//Change terminal settings or return -1 if failed.
int changeTermSettings(struct termios);

#endif
