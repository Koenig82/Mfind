CC=gcc
CFLAGS= -std=c11 -lpthread -D_GNU_SOURCE

all: mfind

mfind: mfind.o queue.o list_2cell.o 
	$(CC) $(CFLAGS) -o mfind -g list_2cell.o mfind.o queue.o

mfind.o: mfind.c mfind.h
	 $(CC) $(CFLAGS) -c mfind.c

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

list_2cell.o: list_2cell.c list_2cell.h
	$(CC) $(CFLAGS) -c list_2cell.c

clean:
	 rm -f rm *.o
