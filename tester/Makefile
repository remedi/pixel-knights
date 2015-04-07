CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic
LFLAGS=-pthread

default: tester

debug: tester.c reader.c ../maps/maps.c ../address.c
	$(CC) $(CFLAGS) $(LFLAGS) -g -o tester tester.c reader.c ../maps/maps.c ../address.c

tester: tester.o reader.o ../maps/maps.o ../address.o
	$(CC) $(CFLAGS) $(LFLAGS) -o tester tester.o reader.o ../maps/maps.o ../address.o

tester.o: tester.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o tester.o tester.c

reader.o: reader.c
	$(CC) $(CFLAGS) $(LFLAGS) -c -o reader.o reader.c

clean:
	$(RM) tester *.o *.gch
