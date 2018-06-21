#include "logging.h"

void sendWarning(char c[]){
        printf(YELLOW "Warning: " RESET "%s\n", c);
}

void sendError(char c[]){
        printf(RED "Fatal Error: " RESET "%s\n", c);
        printf(RED "Program terminated.\n" RESET);
        exit(-1);
}

void printRegisters(){
        bool inInterrupt = ((readFromRegister(6)&0x20) != 0) ? true : false;
        (inInterrupt) ? printf("i") : 1 ;
        printf("r0       : 0x%04hx\n", readFromRegister(0));
        (inInterrupt) ? printf("i") : 1 ;
        printf("r1       : 0x%04hx\n", readFromRegister(1));
        (inInterrupt) ? printf("i") : 1 ;
        printf("r2       : 0x%04hx\n", readFromRegister(2));
        (inInterrupt) ? printf("i") : 1 ;
        printf("r3       : 0x%04hx\n", readFromRegister(3));
        (inInterrupt) ? printf("i") : 1 ;
        printf("r4       : 0x%04hx\n", readFromRegister(4));
        (inInterrupt) ? printf("i") : 1 ;
        printf("r5 (LINK): 0x%04hx\n", readFromRegister(5));
        printf("r6 (PSW) : 0x%04hx ", readFromRegister(6));
        printPSW(readFromRegister(6));
        printf("r7 (PC)  : 0x%04hx\n", readFromRegister(7));
}

void printMem(unsigned char * mem, int memSize, int beginning, int end) {
        if (end == 0x8000) end = 0x7FFF;
        // Let's try not the access outside of what we want :)
        if (beginning - end >= 0 ) return;
        if (end > memSize){
                sendWarning ("End address goes beyond the memory size. Command not executed.");
                return;
        }
        if (beginning < 0 ){
                sendWarning ("Start address doesn't belong to memory address. Command not executed.");
                return;
        }

        printf("         00   02   04   06   08   0a   0c   0e\n");
        int printed = 0;
        for (int i = beginning; i <= end; i+=0x2){
                if (printed == 0){
                        printf("0x%04x: ", i);
                        printed = 0;
                }
                printf("%02x%02x ", mem[i], mem[i+1]);
                printed++;
                if (printed == 8 || i == end){
                        printed = 0;
                        printf("\n");
                }
        }
}

void printPSW(short int PSW){
        bool BS = (PSW & 0b100000) >> 5;
        bool IE = (PSW & 0b10000) >> 4;
        bool P  = (PSW & 0b1000) >> 3;
        bool GE = (PSW & 0b100) >> 2;
        bool CY = (PSW & 0b10) >> 1;
        bool Z  = PSW & 0b1;
        printf("(BS = %d; IE = %d; P = %d; GE = %d; CY = %d; Z = %d)\n", BS, IE, P, GE, CY, Z);
        return;
}

char * toLowerArray(char * array, int sizeArray){
        for (int i = 0; i < sizeArray; i++) {
                if (array[i] == '\n') break;
                array[i] = tolower(array[i]);
        }
        return array;
}
