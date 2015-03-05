CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic

default: server

server: server.o gamestate.o
	$(CC) $(CFLAGS) -o server server.o gamestate.o

test: test.o gamestate.o
	$(CC) $(CFLAGS) -o test test.o gamestate.o

server.o: server.c
	$(CC) $(CFLAGS) -c -o server.o server.c

gamestate.o: gamestate.c
	$(CC) $(CFLAGS) -c -o gamestate.o gamestate.c

test.o: test.c
	$(CC) $(CFLAGS) -c -o test.o test.c

clean:
	$(RM) server test *.o
