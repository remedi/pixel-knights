#ifndef CLIENT_H
#define CLIENT_H


typedef enum {
	UP,
	DOWN,
	RIGHT,
	LEFT,
	ATTACK,
	SHOOT
} Action;

// Functions:

//Parse ip and port from character strings to a struct sockaddr_in.
struct sockaddr_in ipv4_parser(char *, char *);

// Get character from terminal

char getInput(char *);

// Handle received character
char *processCommand(char, char, char *);

#endif
