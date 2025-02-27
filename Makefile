CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -pedantic 
RM = rm -f

default: all

all: main

main: main.c 
	$(CC) $(CFLAGS) -o g502-lightcontroller main.c -lusb-1.0

clean:
	$(RM) main
