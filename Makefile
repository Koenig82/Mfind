CC = gcc

SOURCES = \
mfind.c \
queue.c \
list_2cell.c

INCLUDES = \

LIBRARIES = \
-lpthread

FLAGS = \
-g

BINARY = \
mfind

all:
	$(CC) $(FLAGS) $(SOURCES) $(INCLUDES) $(LIBRARIES) -o $(BINARY)

clean:
	rm ./$(BINARY)
