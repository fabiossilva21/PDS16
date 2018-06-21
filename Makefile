.PHONY: build test clean
#declarar variaveis
CC=gcc
CFLAGS=-lm -Wall -lpthread -lncurses

build:
	$(CC) -o build/PDS16 src/cpu.c src/logging.c src/microcode.c src/main.c $(CFLAGS)

test: build
	./build/PDS16 a.hex

clean:
	rm -rf bin/
