CC = gcc
CFLAGS = -Wall -Wextra -O2

all: patcher

patcher: pa5.o
	$(CC) $(CFLAGS) -o patcher pa5.o

pa5.o: pa5.c
	$(CC) $(CFLAGS) -c pa5.c

clean:
	rm -f *.o patcher
