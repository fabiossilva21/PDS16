#include "logging.h"
#include <pthread.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#define ADDRESS_BITS    16
enum { MEMSIZE = (int)pow(2, ADDRESS_BITS-1) };
#define NUM_REGISTERS   8
#define NUM_IREGISTERS  5
char lastcommand[255];

pthread_t tids[2];
bool runToBeKilled;

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
        (byte & 0x8000 ? '1' : '0'), \
        (byte & 0x4000 ? '1' : '0'), \
        (byte & 0x2000 ? '1' : '0'), \
        (byte & 0x1000 ? '1' : '0'), \
        (byte & 0x0800 ? '1' : '0'), \
        (byte & 0x0400 ? '1' : '0'), \
        (byte & 0x0200 ? '1' : '0'), \
        (byte & 0x0100 ? '1' : '0'), \
        (byte & 0x80 ? '1' : '0'), \
        (byte & 0x40 ? '1' : '0'), \
        (byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

typedef struct {
        unsigned char mem[MEMSIZE];
        short int registers[NUM_REGISTERS];
        short int iregisters[NUM_IREGISTERS];
} PDS16;

PDS16 pds16;

void loop();
int decodeOp(unsigned int code);
void *killThread();
void *run();
void initializePDS16();
int parseHexFile(unsigned char * mem, FILE *fileopened);
short int readFromRegister(int registerID);
void writeToRegister(int registerID, short int value);
void enterInterruption();
void exitInterruption();
