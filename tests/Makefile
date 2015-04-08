CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic -pthread

default: all

all: gamestate

gamestate:
	$(CC) $(CFLAGS) -o gamestate_test gamestate_test.c ../server/gamestate.c

clean:
	$(RM) gamestate_test *.o *.gch
