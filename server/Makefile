CC=gcc
CFLAGS=-std=gnu11 -Wall -Wextra -pedantic

default: server

server.o: server.c
	$(CC) $(CFLAGS) -o server.o -c server.c

server: server.o
	$(CC) $(CFLAGS) -o server server.o

clean:
	$(RM) server *.o
