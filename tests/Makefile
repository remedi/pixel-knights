CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic -pthread

default: all

all: gamestate network

gamestate:
	$(CC) $(CFLAGS) -o gamestate_test gamestate_test.c ../server/gamestate.c

network:
	$(CC) $(CFLAGS) -o network_test network_test.c

clean:
	$(RM) gamestate_test *.o *.gch
