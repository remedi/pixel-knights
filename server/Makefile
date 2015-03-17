CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic -pthread

default: server

debug: server.c gamestate.c thread.c main.c
	$(CC) $(CFLAGS) -o server -g server.c gamestate.c thread.c main.c

server: server.o gamestate.o thread.o main.o
	$(CC) $(CFLAGS) -o server server.o gamestate.o thread.o main.o

test: test.o gamestate.o
	$(CC) $(CFLAGS) -o test test.o gamestate.o

main.o: main.c
	$(CC) $(CFLAGS) -c -o main.o main.c

server.o: server.c
	$(CC) $(CFLAGS) -c -o server.o server.c

gamestate.o: gamestate.c
	$(CC) $(CFLAGS) -c -o gamestate.o gamestate.c

thread.o: thread.c
	$(CC) $(CFLAGS) -c -o thread.o thread.c

test.o: test.c
	$(CC) $(CFLAGS) -c -o test.o test.c

clean:
	$(RM) server test *.o *.gch
