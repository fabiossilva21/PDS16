#include "microcode.h"

bool breakpointHit = false;

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

void writeToRam(unsigned char * mem, char * Line, int addressToWrite){
        int limit = (addressToWrite & 0xff0000) >> 16;
        addressToWrite = addressToWrite & 0xffff;
        printf("Writing %d bytes to RAM Address: %04x\n", limit, addressToWrite);
        for (int i = 8; i < limit*2+8; i+=2){
                pds16.mem[addressToWrite] = Line[i]*16+Line[i+1];
                printf("%02x ", pds16.mem[addressToWrite]);
                addressToWrite++;
        }
        printf("\n\n");
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
        memset(&breakpointCounter, 0, sizeof(int));
}

void *run(){
        printf(GREEN "NOTICE: " RESET "Press - to end the run routine.\n");
        for(;;){
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
                pds16.registers[7]+=2;
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
                        ldi(code);
                        break;
                case 0b00001:
                        ldih(code);
                        break;
                case 0b00010:
                        ld(code);
                        break;
                case 0b00011:
                        ld(code);
                        break;
                case 0b00110:
                        st(code);
                        break;
                case 0b00111:
                        st(code);
                        break;
                case 0b10000:
                        add(code);
                        break;
                case 0b10010:
                        adc(code);
                        break;
                case 0b10001:
                        sub(code);
                        break;
                case 0b10011:
                        sbb(code);
                        break;
                case 0b10100:
                        add(code);
                        break;
                case 0b10110:
                        adc(code);
                        break;
                case 0b10101:
                        sub(code);
                        break;
                case 0b10111:
                        sbb(code);
                        break;
                case 0b11000:
                        anl(code);
                        break;
                case 0b11001:
                        orl(code);
                        break;
                case 0b11010:
                        xrl(code);
                        break;
                case 0b11011:
                        not(code);
                        break;
                case 0b11100:
                        shl(code);
                        break;
                case 0b11101:
                        shr(code);
                        break;
                case 0b11110:
                        rr(code);
                        break;
                case 0b01000:
                        jz(code);
                        break;
                case 0b01001:
                        jnz(code);
                        break;
                case 0b01010:
                        jc(code);
                        break;
                case 0b01011:
                        jnc(code);
                        break;
                case 0b01100:
                        jmp(code);
                        break;
                case 0b01101:
                        jmpl(code);
                        break;
                case 0b01110:
                        iret();
                        break;
                case 0b01111:
                        break;
                default:
                        printf(RED "OpCode not recognized %x\n" RESET, opCode);
                        exit(-1);
        }
        return 0;
}
