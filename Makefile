.PHONY: build test clean

#declarar variaveis
CC=gcc
CLIBRARIES=-lssl -lcrypto -lm -lpthread -lncurses
CARGS= -std=c99 -Wall $(CLIBRARIES)

build: clean
	mkdir -p build/
	$(CC) -o build/PDS16 src/cpu.c src/logging.c src/microcode.c src/main.c src/gui.c $(CARGS)

test: build
	./build/PDS16 a.hex

clean:
	rm -rf bin/PDS16
