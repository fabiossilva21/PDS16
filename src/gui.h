#define _GNU_SOURCE
#include <stdio.h>
#include <term.h>
#include <stdlib.h>
#include <string.h>
#include "microcode.h"
#include <openssl/sha.h>


bool fixedRegisters;
bool fixedASM;
bool clearScreenEveryCommand;
bool dontPrintAnything;
char lastcommand[255];
pthread_t tids[2];
unsigned char sha1[SHA_DIGEST_LENGTH];
