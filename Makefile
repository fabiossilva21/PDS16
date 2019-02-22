.PHONY: build test clean

#declarar variaveis
CC=gcc
WCC=x86_64-w64-mingw32-gcc
CLIBRARIES= -lssl -lcrypto -lm -lpthread -lncurses
CARGS= -std=c99 -Wall -Wextra $(CLIBRARIES)

build: clean
	mkdir -p build/
	$(CC) -g -o build/PDS16 src/cpu.c src/logging.c src/microcode.c src/main.c src/ui.c src/gui.c $(CARGS)

test: build
	./build/PDS16 a.hex

clean:
	rm -rf build/PDS16
