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
        printf("r7 (PC)  : 0x%04hx  --> (0x%04x)\n", readFromRegister(7), (readFromRam(readFromRegister(7))<<8) + readFromRam(readFromRegister(7)+1));
}

void printMem(unsigned char * mem, int memSize, int beginning, int end, unsigned char nCS_Out) {
        int printed = 0;
        if (end == 0x8000) end = 0x7FFF;
        // Let's try not the access outside of what we want :)
        if (beginning - end >= 0 ) return;
        if ((end > memSize && end < 0xFEFF) || (end > memSize && end > 0xFF3F)){
                sendWarning ("End address goes beyond the memory size. Command not executed.");
                return;
        }
        if (beginning < 0 ){
                sendWarning ("Start address doesn't belong to memory address. Command not executed.");
                return;
        }
        printf("         00   02   04   06   08   0a   0c   0e\n");
        if (beginning > 0xFEFF && end < 0xFF40 ){
                for (int i = beginning; i <= end; i+=0x2){
                        if (printed == 0){
                                printf("0x%04x: ", i);
                                printed = 0;
                        }
                        printf("%02x%02x ", nCS_Out, nCS_Out);
                        printed++;
                        if (printed == 8 || i == end){
                                printed = 0;
                                printf("\n");
                        }
                }
                return;
        }
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

void printOp(int code, int memoryAddress){
        if (isOnBreakpointList(memoryAddress)){
                printf("\t*0x%04x: ", memoryAddress);
        }else{
                printf("\t 0x%04x: ", memoryAddress);
        }

        int opCode = (code & (0b11111<<11))>>11;

        char name[255] = {0};
        bool symbolsFound = false;

        // Some declarations
        int rb1 = (code&0b111);
        int rb2 = (code&0b111000)>>3;
        int rn = (code&0b111000000)>>6;
        int const4 = (code&0b1111000000)>>6;
        int id = rn, ri = rn;
        int rm = rb2;
        int rd = rb1, rs = rb1;

        bool w = (code>>10)&1;
        bool r = (code>>9)&1;
        bool f = w, si = w;

        int8_t dir7 = (code>>3)&0x7F;
        int8_t off8 = (code>>3)&0xFF;
        int8_t imm8 = off8;


        switch (opCode) {
                case 0b00000:
                        printf("ldi\t r%d, #" YELLOW "0x%02hhx" RESET, rd, imm8);
                        break;
                case 0b00001:
                        printf("ldih\t r%d, #" YELLOW "0x%02hhx" RESET, rd, imm8);
                        break;
                case 0b00010:
                        if(!w){
                                printf("ldb\t ");
                        }else{
                                printf("ld\t ");
                        }
                        printf("r%d," RED " 0x%02hhx" RESET, rd, dir7);
                        break;
                case 0b00011:
                        if(!w){
                                printf("ldb\t ");
                        }else{
                                printf("ld\t ");
                        }
                        if((code&0x100) != 0){
                                printf("r%d, [r%d, r%d]", rd, rb2, ri);
                        }else{
                                printf("r%d, [r%d, #" YELLOW "0x%x" RESET "]", rd, rb2, id);
                        }
                        break;
                case 0b00110:
                        if(!w){
                                printf("stb\t ");
                        }else{
                                printf("st\t ");
                        }
                        printf("r%d," RED " 0x%02hhx" RESET, rs, dir7);
                        break;
                case 0b00111:
                        if(!w){
                                printf("stb\t ");
                        }else{
                                printf("st\t ");
                        }
                        if((code&0x100) != 0){
                                printf("r%d, [r%d, r%d]", rs, rb2, ri);
                        }else{
                                printf("r%d, [r%d, #" YELLOW "0x%x" RESET "]", rs, rb2, id);
                        }
                        break;
                case 0b10000:
                        printf("add");
                        if (!r){
                                printf("r");
                        }
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d, r%d", rd, rm, rn);
                        break;
                case 0b10010:
                        printf("adc");
                        if (!r){
                                printf("r");
                        }
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d, r%d", rd, rm, rn);
                        break;
                case 0b10001:
                        printf("sub");
                        if (!r){
                                printf("r");
                        }
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d, r%d", rd, rm, rn);
                        break;
                case 0b10011:
                        printf("sbb");
                        if (!r){
                                printf("r");
                        }
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d, r%d", rd, rm, rn);
                        break;
                case 0b10100:
                        printf("add");
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d, #" YELLOW "0x%02x" RESET, rd, rm, const4);
                        break;
                case 0b10110:
                        printf("adc");
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d, #" YELLOW "0x%02x" RESET, rd, rm, const4);
                        break;
                case 0b10101:
                        printf("sub");
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d, #" YELLOW "0x%02x" RESET, rd, rm, const4);
                        break;
                case 0b10111:
                        printf("sbb");
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d, #" YELLOW "0x%02x" RESET, rd, rm, const4);
                        break;
                case 0b11000:
                        printf("anl");
                        if (!r){
                                printf("r");
                        }
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d, r%d", rd, rm, rn);
                        break;
                case 0b11001:
                        printf("orl");
                        if (!r){
                                printf("r");
                        }
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d, r%d", rd, rm, rn);
                        break;
                case 0b11010:
                        printf("xrl");
                        if (!r){
                                printf("r");
                        }
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d, r%d", rd, rm, rn);
                        break;
                case 0b11011:
                        printf("not");
                        if (!r){
                                printf("r");
                        }
                        if (!f){
                                printf("f");
                        }
                        printf("\t r%d, r%d", rd, rm);
                        break;
                case 0b11100:
                        printf("shl\t r%d, r%d, #" YELLOW "0x%02x" RESET ", %d", rd, rm, const4, si);
                        break;
                case 0b11101:
                        printf("shr\t r%d, r%d, #" YELLOW "0x%02x" RESET ", %d", rd, rm, const4, si);
                        break;
                case 0b11110:
                        if(f){
                                printf("rrm\t ");
                        }else{
                                printf("rrl\t ");
                        }
                        printf("r%d, r%d, #" YELLOW "0x%02x" RESET, rd, rm, const4);
                        break;
                case 0b01000:
                        for (int i = 0; i < symbols.addressesIn; i++){
                                if (symbols.n_address[i] == memoryAddress + off8) {
                                        strcpy(name, symbols.addressNames[i]);
                                        symbolsFound = true;
                                        break;
                                }
                        }
                        printf("jz\t r%d," , rb1);
                        if (!symbolsFound){
                                printf(CYAN " 0x%02hhx" RESET, off8*2);
                        }else{
                                printf(GREEN " %s", name);
                        }
                        break;
                case 0b01001:
                        for (int i = 0; i < symbols.addressesIn; i++){
                                if (symbols.e_address[i] == memoryAddress + off8*2+2) {
                                        strcpy(name, symbols.addressNames[i]);
                                        symbolsFound = true;
                                        break;
                                }
                        }
                        printf("jnz\t");
                        if (!symbolsFound){
                                printf( " r%d," CYAN " 0x%02hhx" RESET, rb1, off8*2+2);
                        }else{
                                printf(GREEN " %s" RESET, name);
                        }
                        break;
                case 0b01010:
                        for (int i = 0; i < symbols.addressesIn; i++){
                                if (symbols.e_address[i] == memoryAddress + off8*2+2) {
                                        strcpy(name, symbols.addressNames[i]);
                                        symbolsFound = true;
                                        break;
                                }
                        }
                        printf("jc\t");
                        if (!symbolsFound){
                                printf( " r%d," CYAN " 0x%02hhx" RESET, rb1, off8*2+2);
                        }else{
                                printf(GREEN " %s" RESET, name);
                        }
                        break;
                case 0b01011:
                        for (int i = 0; i < symbols.addressesIn; i++){
                                if (symbols.e_address[i] == memoryAddress + off8*2+2) {
                                        strcpy(name, symbols.addressNames[i]);
                                        symbolsFound = true;
                                        break;
                                }
                        }
                        printf("jnc\t");
                        if (!symbolsFound){
                                printf( " r%d," CYAN " 0x%02hhx" RESET, rb1, off8*2+2);
                        }else{
                                printf(GREEN " %s" RESET, name);
                        }
                        break;
                case 0b01100:
                        for (int i = 0; i < symbols.addressesIn; i++){
                                if (symbols.e_address[i] == memoryAddress + off8*2+2) {
                                        strcpy(name, symbols.addressNames[i]);
                                        symbolsFound = true;
                                        break;
                                }
                        }
                        printf("jmp\t");
                        if (!symbolsFound){
                                printf( " r%d," CYAN " 0x%02hhx" RESET, rb1, off8*2+2);
                        }else{
                                printf(GREEN " %s" RESET, name);
                        }
                        break;
                case 0b01101:
                        for (int i = 0; i < symbols.addressesIn; i++){
                                if (symbols.e_address[i] == memoryAddress + off8*2+2) {
                                        strcpy(name, symbols.addressNames[i]);
                                        symbolsFound = true;
                                        break;
                                }
                        }
                        printf("jmpl\t");
                        if (!symbolsFound){
                                printf( " r%d," CYAN " 0x%02hhx" RESET, rb1, off8*2+2);
                        }else{
                                printf(GREEN " %s" RESET, name);
                        }
                        break;
                case 0b01110:
                        printf("iret");
                        break;
                case 0b01111:
                        printf("nop");
                        break;
                default:
                        printf(YELLOW "OpCode not recognized 0x%x" RESET, opCode);
        }
        if (readFromRegister(7) == memoryAddress){
                printf(" <---\n");
        }else{
                printf("\n");
        }
        return;
}
