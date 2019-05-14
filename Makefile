NAME=proj2
CC=gcc
CFLAGS= -std=gnu99 -Wall -Wextra -Werror -pedantic -g -lpthread -lrt

all:$(NAME)

proj2:proj2.c
	$(CC) -o proj2 proj2.c $(CFLAGS)
clean:
	rm proj2 proj2.out
zip:
	zip proj2.zip proj2.c Makefile
