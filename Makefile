CC = gcc

CFLAGS= -std=c11 -Wall -Wextra -Werror -Wmissing-declarations -Wmissing-prototypes -Werror-implicit-function-declaration -Wreturn-type -Wparentheses -Wunused -Wold-style-definition -Wundef -Wshadow -Wstrict-prototypes -Wswitch-default -Wstrict-prototypes -Wunreachable-code -g -std=c11 -D_GNU_SOURCE

SOURCES = \
mfind.c \
queue.c \
list_2cell.c

INCLUDES = \

LIBRARIES = \
-lpthread\

BINARY = \
mfind

all:
	$(CC) $(FLAGS) $(SOURCES) $(INCLUDES) $(LIBRARIES) -o $(BINARY)

clean:
	rm ./$(BINARY)
