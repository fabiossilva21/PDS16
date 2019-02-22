#ifndef _UI_H_
#define _UI_H_

#define _GNU_SOURCE
#include <stdio.h>
#include <term.h>
#include <stdlib.h>
#include <string.h>
#include "microcode.h"
#include <openssl/sha.h>

typedef struct{
    char * command;
    char * args;
}Input;

typedef struct CommandHandler{
    struct CommandHandler * next;
    char * command;
    void (*function_ptr)(Input *);
}CommandHandler;

typedef struct{
	size_t size;
	CommandHandler **table;
} Hashtable;

bool fixedRegisters;
bool fixedASM;
bool clearScreenEveryCommand;
bool dontPrintAnything;
char lastcommand[255];
pthread_t tids[2];
unsigned char sha1[SHA_DIGEST_LENGTH];

void printHelp(char * commandHelp);

#endif