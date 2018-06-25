#include <stdio.h>
#include <term.h>
#include <stdlib.h>
#include <string.h>
#include "logging.h"
#include "microcode.h"

bool fixedRegisters;
bool fixedASM;
bool clearScreenEveryCommand;
bool dontPrintAnything;
char lastcommand[255];
pthread_t tids[2];
