CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic
LFLAGS=-pthread

default: client

debug: client.c update.c ../maps/maps.c
	$(CC) $(CFLAGS) $(LFLAGS) -g -o client client.c update.c ../maps/maps.c

client: client.o update.o maps
	$(CC) $(CFLAGS) $(LFLAGS) -o client client.o update.o ../maps/maps.o

client.o: client.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o client.o client.c

update.o: update.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o update.o update.c

maps: ../maps/maps.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o ../maps/maps.o ../maps/maps.c

clean:
	$(RM) client *.o *.gch
