#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define RESET   "\e[0m"
#define RED     "\e[1;31m"
#define GREEN   "\e[1;32m"
#define YELLOW  "\e[1;33m"
#define CYAN    "\e[1;36m"

void sendWarning(char c[]);
void sendError(char c[]);
void printMem(unsigned char * mem, int memSize, int beginning, int end);
void printRegisters(unsigned short int * registers);
void printPSW(unsigned short int PSW);
