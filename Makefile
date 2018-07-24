.PHONY: build test clean

#declarar variaveis
CC=gcc
CLIBRARIES=-lssl -lcrypto -lm -lpthread -lncurses
CARGS= -std=c99 -Wall $(CLIBRARIES)

build: clean
	mkdir -p build/
	$(CC) -o build/PDS16 src/cpu.c src/logging.c src/microcode.c src/main.c src/gui.c src/gui-windows.c $(CARGS)

gt: clean
	mkdir -p build/
	$(CC) -o build/gt src/gui-windows.c -Wall

gtk-gui:
	$(CC) gtk/gtk_gui.c -o build/guiwin -Wall `pkg-config --cflags --libs gtk+-3.0` -export-dynamic

test: build
	./build/PDS16 a.hex

clean:
	rm -rf bin/PDS16
