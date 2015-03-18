CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic
LFLAGS=-pthread

default: client

debug: client.c local.c update.c
	$(CC) $(CFLAGS) $(LFLAGS) -g -o client client.c local.c update.c

client: client.o local.o update.o
	$(CC) $(CFLAGS) $(LFLAGS) -o client client.o local.o update.o

client.o: client.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o client.o client.c

local.o: local.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o local.o local.c

update.o: update.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o update.o update.c

clean:
	$(RM) client *.o *.gch
