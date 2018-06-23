#include "microcode.h"

bool breakpointHit = false;

float timedifference_msec(struct timeval t0, struct timeval t1){
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

short int readFromRegister(int registerID){
        if(registerID < 0 || registerID > 7){
                sendError("We just tried to read outside the register bank!");
                printf("PC: %d\n", readFromRegister(7));
                printf("Register ID: %d\n", registerID);
        }
        // Are we in an interrupt routine?
        if ((pds16.registers[6]&0x20) != 0 && registerID < 6){
                return pds16.iregisters[registerID];
        }else{
                return pds16.registers[registerID];
        }
}

void dumpMemory(unsigned char * memory, long unsigned int memSize){
        FILE *fp = fopen("memory.bin", "w+");
        if(!fp) {
                sendError("Could not create memory.bin!");
        }
        sendWarning("Dumping memory to memory.bin...");
        for (int i = 0; i < memSize; i++ ){
                fprintf(fp, "%c", memory[i]);
        }
        sendWarning("Memory dumped sucessfully!");
        printf(GREEN "Dumped: "RESET"%lu bytes\n", (memSize)*sizeof(char));
        fclose(fp);
}

void writeToRegister(int registerID, short int value){
        if(registerID < 0 || registerID > 7){
                printf("Register ID: %d\n", registerID);
                printf("PC: %d\n", readFromRegister(7));
                sendError("We just tried to write outside the register bank!");
        }
        // Are we in an interrupt routine?
        if ((pds16.registers[6]&0x20) != 0 && registerID < 6){
                pds16.iregisters[registerID] = value&0xFFFF;
        }else{
                pds16.registers[registerID] = value&0xFFFF;
        }
}

void enterInterruption(){
        if ((readFromRegister(6)&0x10) != 0){
                // Copy PSW to iR0
                pds16.iregisters[0] = pds16.registers[6];
                // Set BS and set IE to false
                writeToRegister(6, 0x20);
                // Copy PC to LINK
                writeToRegister(5, readFromRegister(7));
                // Put a 2 in PC
                writeToRegister(7, 2);
                sendWarning("Entered the interrupt routine!");
        }else if ((readFromRegister(6) & 0x20) != 0){
                sendWarning("We cannot enter an interrupt routine while being in one!");
        }else{
                sendWarning("Just tried to enter the Interruption routine without IE bit set!");
        }
}

void exitInterruption(){
        if ((readFromRegister(6)&0x20) == 0){
                sendError("We got out of scope and tried to exit an interrupt routine without being in one!");
        }
        // Move LINK to PC
        writeToRegister(7, readFromRegister(5));
        // Move iR0 to PSW
        writeToRegister(6, readFromRegister(0));
        sendWarning("Exited the interrupt routine!");
}

void erasePDS(unsigned char * mem){
        sendWarning("Erasing the RAM...\n");
        int spaces = 30;
        for(int i = 0; i < MEMSIZE; i++){
                printf("\rProcess: [");
                float progress = i/(double)MEMSIZE;
                for(int j = 0; j < progress*spaces; j++){
                        printf("=");
                }
                for(int j = 0; j < (spaces-progress*spaces)-1; j++){
                        if(j == 0){
                                printf(">");
                        }
                        printf(" ");
                }
                printf("] %.2f%%", progress*100);
                mem[i] = 0;
        }
        printf("\nRAM sucessfully erased!\n");

        sendWarning("Resetting the registers!\n");
        for (int i = 0; i < NUM_REGISTERS; i++){
                pds16.registers[i] = 0;
        }
        for (int i = 0; i < NUM_IREGISTERS; i++){
                pds16.iregisters[i] = 0;
        }
        printf("\nRegisters sucessfully resetted!\n");
}

void patchMemory(int address, int value, bool byte){
        if (address > 0x7fff || address < 0x0){
                printf("Cannot write 0x%04x to 0x%04x\n", value,  address);
                return;
        }
        if (byte){
                pds16.mem[address] = value&0xff;
                printf("Sucessfully patched memory address 0x%02x with 0x%02x!\n", address, value&0xff);
        }else{
                address &= 0xfffe; // Let's remove the last byte
                pds16.mem[address] = (value&0xff00)>>8;
                pds16.mem[address+1] = (value&0xff);
                printf("Sucessfully patched memory address 0x%02x and 0x%02x with 0x%02x and 0x%02x!\n", address, address+1, (value&0xff00)>>8, value&0xff);
        }


        return;
}

void writeToRam(unsigned char * mem, char * Line, int addressToWrite){
        int limit = (addressToWrite & 0xff0000) >> 16;
        addressToWrite = addressToWrite & 0xffff;
        printf("Writing %d bytes to RAM Address: 0x%04x\n", limit, addressToWrite);
        for (int i = 8; i < limit*2+8; i+=2){
                pds16.mem[addressToWrite] = Line[i]*16+Line[i+1];
                printf("%02x ", pds16.mem[addressToWrite]);
                addressToWrite++;
        }
        printf("\n");
}

int getVal(char c){
        int rtVal = 0;

        if(c >= '0' && c <= '9'){
           rtVal = c - '0';
        }else{
           rtVal = c - 'A' + 10;
        }
        return rtVal;
}

int getAddressFromLine(char * Line){
        int number = 0;
        number = (Line[5])+(Line[4])*16+(Line[3])*pow(16,2)+(Line[2])*pow(16,3)+
        (Line[1])*pow(16,4)+Line[0]*pow(16,5);
        return number;
}

int parseHexFile(unsigned char * mem, FILE *fileopened){
        int c, i = 0;
        char line[2555];
        while ((c = fgetc(fileopened)) != EOF){
                if(c == ':'){
                        c = fgetc(fileopened);
                }
                if(c != '\n'){
                        line[i] = getVal(c);
                        i++;
                }else{
                        if(line[0] == 0 && line[1] == 0) return 1;
                        int memLoc = getAddressFromLine(line);
                        i = 0;
                        writeToRam(pds16.mem, line, memLoc);
                }
        }
        return 1;
}

void initializePDS16(){
        // Force everything to be 0
        memset(pds16.mem, 0x00, MEMSIZE*sizeof(unsigned char));
        memset(pds16.registers, 0x00, NUM_REGISTERS*sizeof(unsigned short));
        memset(pds16.iregisters, 0x00, NUM_IREGISTERS*sizeof(unsigned short));
        memset(breakpoints, 0xFFFFFFFF, (MAX_BREAKPOINTS-1)*sizeof(int));
        memset(&interruptTime, -1, sizeof(int));
}

void *run(){
        struct timeval t0;
        struct timeval t1;
        gettimeofday(&t0, 0);
        printf(GREEN "NOTICE: " RESET "Press - to end the run routine.\n");
        for(;;){
                gettimeofday(&t1, 0);
                if (interruptTime != -1){
                        if (timedifference_msec(t0, t1) > interruptTime){
                                gettimeofday(&t0, 0);
                                enterInterruption();
                        }
                }
                for (int i = 0; i < MAX_BREAKPOINTS-1; i++){
                        if (readFromRegister(7) == breakpoints[i]){
                                breakpointHit = true;
                                printf(RED "\n\nHit breakpoint #%d at memory address: 0x%04x\n" RESET, i, breakpoints[i]);
                                printf("\n\nPress ENTER to continue\n");
                        }
                }
                if(runToBeKilled || breakpointHit){
                        runToBeKilled = false;
                        pthread_exit(NULL);
                }
                int opp = (pds16.mem[pds16.registers[7]]<<8)+pds16.mem[pds16.registers[7]+1];
                decodeOp(opp);
        }
}

void *killThread(){
        char ch[255];
        while(1){
                if(fgets(ch, sizeof(ch), stdin) != NULL){
                        ch[0] = tolower(ch[0]);
                        if(ch[0] == '-'){
                                runToBeKilled = true;
                                while(runToBeKilled){

                                }
                                pthread_exit(NULL);
                        }else if(ch[0] == 'r'){
                                printRegisters(pds16.registers);
                        }else if (breakpointHit){
                                pthread_exit(NULL);
                        }else{
                                sendWarning("Running in auto mode. No commands accepted. Press - to exit or 'r' to see the registers");
                        }
                }
        }
}

int decodeOp(unsigned int code){
        int opCode = (code & (0b11111<<11))>>11;
        switch (opCode) {
                case 0b00000:
                        writeToRegister(7, readFromRegister(7)+2);
                        ldi(code);
                        break;
                case 0b00001:
                        writeToRegister(7, readFromRegister(7)+2);
                        ldih(code);
                        break;
                case 0b00010:
                        writeToRegister(7, readFromRegister(7)+2);
                        ld(code);
                        break;
                case 0b00011:
                        writeToRegister(7, readFromRegister(7)+2);
                        ld(code);
                        break;
                case 0b00110:
                        writeToRegister(7, readFromRegister(7)+2);
                        st(code);
                        break;
                case 0b00111:
                        writeToRegister(7, readFromRegister(7)+2);
                        st(code);
                        break;
                case 0b10000:
                        writeToRegister(7, readFromRegister(7)+2);
                        add(code);
                        break;
                case 0b10010:
                        writeToRegister(7, readFromRegister(7)+2);
                        adc(code);
                        break;
                case 0b10001:
                        writeToRegister(7, readFromRegister(7)+2);
                        sub(code);
                        break;
                case 0b10011:
                        writeToRegister(7, readFromRegister(7)+2);
                        sbb(code);
                        break;
                case 0b10100:
                        writeToRegister(7, readFromRegister(7)+2);
                        add(code);
                        break;
                case 0b10110:
                        writeToRegister(7, readFromRegister(7)+2);
                        adc(code);
                        break;
                case 0b10101:
                        writeToRegister(7, readFromRegister(7)+2);
                        sub(code);
                        break;
                case 0b10111:
                        writeToRegister(7, readFromRegister(7)+2);
                        sbb(code);
                        break;
                case 0b11000:
                        writeToRegister(7, readFromRegister(7)+2);
                        anl(code);
                        break;
                case 0b11001:
                        writeToRegister(7, readFromRegister(7)+2);
                        orl(code);
                        break;
                case 0b11010:
                        writeToRegister(7, readFromRegister(7)+2);
                        xrl(code);
                        break;
                case 0b11011:
                        writeToRegister(7, readFromRegister(7)+2);
                        not(code);
                        break;
                case 0b11100:
                        writeToRegister(7, readFromRegister(7)+2);
                        shl(code);
                        break;
                case 0b11101:
                        writeToRegister(7, readFromRegister(7)+2);
                        shr(code);
                        break;
                case 0b11110:
                        writeToRegister(7, readFromRegister(7)+2);
                        rr(code);
                        break;
                case 0b01000:
                        writeToRegister(7, readFromRegister(7)+2);
                        jz(code);
                        break;
                case 0b01001:
                        writeToRegister(7, readFromRegister(7)+2);
                        jnz(code);
                        break;
                case 0b01010:
                        writeToRegister(7, readFromRegister(7)+2);
                        jc(code);
                        break;
                case 0b01011:
                        writeToRegister(7, readFromRegister(7)+2);
                        jnc(code);
                        break;
                case 0b01100:
                        writeToRegister(7, readFromRegister(7)+2);
                        jmp(code);
                        break;
                case 0b01101:
                        writeToRegister(7, readFromRegister(7)+2);
                        jmpl(code);
                        break;
                case 0b01110:
                        iret();
                        break;
                case 0b01111:
                        writeToRegister(7, readFromRegister(7)+2);
                        break;
                default:
                        printf(RED "OpCode not recognized %x\n" RESET, opCode);
                        exit(-1);
        }
        return 0;
}
