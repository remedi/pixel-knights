CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic -pthread -g
LFLAGS=

default: client

test: client.o local.o
	$(CC) $(CFLAGS) -o client client.o local.o

client: client.o local.o
	$(CC) $(CFLAGS) -o client client.o local.o

client.o: client.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o client.o client.c

local.o: local.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o local.o local.c

clean:
	$(RM) client *.o *.gch
