#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>
#include "interconnect.h"

#define RESET   "\e[0m"
#define LIGHT_RED "\e[0;31m"
#define RED     "\e[1;31m"
#define GREEN   "\e[1;32m"
#define LIGHT_YELLOW "\e[0;33m"
#define YELLOW  "\e[1;33m"
#define CYAN    "\e[1;36m"

typedef struct {
        short int e_address[255];
        short int n_address[255];

        char addressNames[255][255];
        char numericNames[255][255];

        int addressesIn;
        int numericsIn;
} symbolsStruct;

#define MAX_BREAKPOINTS 6
int breakpoints[MAX_BREAKPOINTS];

symbolsStruct symbols;
