#include "microcode.h"
#include <ncurses.h>

char lastcommand[255];
pthread_t tids[2];
