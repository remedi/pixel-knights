CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic
LFLAGS=-pthread

default: client

debug: client.c update.c ../maps/maps.c ../address.c
	$(CC) $(CFLAGS) $(LFLAGS) -g -o client client.c update.c ../maps/maps.c ../address.c

client: client.o update.o ../maps/maps.o ../address.o
	$(CC) $(CFLAGS) $(LFLAGS) -o client client.o update.o ../maps/maps.o ../address.o

client.o: client.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o client.o client.c

update.o: update.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o update.o update.c

maps.o: ../maps/maps.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o ../maps/maps.o ../maps/maps.c

address.o: ../address.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o ../address.o ../address.c

clean:
	$(RM) client *.o *.gch
