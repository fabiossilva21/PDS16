.PHONY: build test clean

#declarar variaveis
CC=gcc
CFLAGS=-lssl -lcrypto -std=c99 -lm -Wall -lpthread -lncurses

build: clean
	mkdir -p build/
	$(CC) -o build/PDS16 src/cpu.c src/logging.c src/microcode.c src/main.c src/gui.c $(CFLAGS)

test: build
	./build/PDS16 a.hex

clean:
	rm -rf bin/PDS16
