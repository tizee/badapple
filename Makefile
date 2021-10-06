
CC = gcc
CFLAGS = -g -Wall -Wextra -std=c99 -pedantic -Wwrite-strings

all: badapple

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

badapple: src/badapple.o
	$(CC) $(CFLAGS) -o $@ $+

clean:
	rm -f src/*.o

mrproper: clean
	rm -f badapple

install: all
	install badapple /usr/local/bin/${package}

.PHONY: all mrproper clean
