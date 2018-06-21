#include "microcode.h"

int breakpoints[MAX_BREAKPOINTS-1] = {0xFFFFFFFF};
int breakpointCounter = 0;
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
        printf(GREEN "Dumped: "RESET"%d bytes\n", (memSize)*sizeof(char));
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
                // Set BS
                writeToRegister(6, 0x20);
                // Set IE false
                writeToRegister(6, readFromRegister(6)^0x10);
                //pds16.registers[6] ^= 0x10;
                // Copy PC to LINK
                writeToRegister(5, readFromRegister(7));
                // Put a 2 in PC
                writeToRegister(7, 2);
        }else{
                sendWarning("Just tried to enter the Interruption routine without IE bit set!");
        }
}

void exitInterruption(){
        // Move LINK to PC
        writeToRegister(7, readFromRegister(5));
        // Move iR0 to PSW
        writeToRegister(6, readFromRegister(0));
        // Set IE back to true
        pds16.registers[6] ^= 0x10;
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
        memset(breakpoints, 0xFFFFFFFF, MAX_BREAKPOINTS*sizeof(int));
}

void *run(){
        printf(GREEN "NOTICE: " RESET "Press - to end the run routine.\n");
        for(;;){
                for (int i = 0; i < breakpointCounter; i++){
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
                                // loop();
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
        //printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(code));
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

void loop(){
        printf("\n0x%04x>", readFromRegister(7));
        char input[255] = {0};
        char option[10] = {0};
        int int1;
        int int2;
        if (fgets(input, sizeof(input), stdin) != NULL){
                if(input[0] != '\n'){
                        strcpy(lastcommand, input);
                }else{
                        strcpy(input, lastcommand);
                }

                if(input[0] == 'e'){
                        sendWarning("Exiting...\n");
                        exit(1);
                }
                // Commands with "string Hnumber Hnumber"
                if(sscanf(input, "%s 0x%x 0x%x", option, &int1, &int2) == 3){
                        if (input[0] == 'd'){
                                printMem(pds16.mem, MEMSIZE, int1, int2);
                                loop();
                        }
                }
                // Commands with "string Dnumber Dnumber"
                if(sscanf(input, "%s %d %d", option, &int1, &int2) == 3){
                        if (input[0] == 'd'){
                                printMem(pds16.mem, MEMSIZE, int1, int2);
                                loop();
                        }
                }
                // Commands with "string charDnumber Hnumber"
                if (sscanf(input, "%s %c%d 0x%x", option, option, &int1, &int2) == 4) {
                        if (input[0] == 's' && input[1] == 'r'){
                                // Set registers
                                if(int1 > 7 || int1 < 0){
                                        sendWarning("Tried to set a non-existant register.\n");
                                        loop();
                                }
                                pds16.registers[int1] = int2;
                                loop();
                        }
                }
                // Commands with "string charDnumber Dnumber"
                if (sscanf(input, "%s %c%d %d", option, option, &int1, &int2) == 4) {
                        if (input[0] == 's' && input[1] == 'r'){
                                if(int1 > 7 || int1 < 0){
                                        sendWarning("Tried to set a non-existant register.\n");
                                        loop();
                                }
                                pds16.registers[int1] = int2;
                                loop();
                        }
                }
                // Commands with "string string Hnumber"
                if (sscanf(input, "%s %s 0x%x", option, option, &int1) == 3){
                        if (input[0] == 'd' && input[2] == '*'){
                                // Print memory from PC to int1
                                printMem(pds16.mem, MEMSIZE, pds16.registers[7], int1);
                                loop();
                        }else if (input[0] == 'b'){
                                if(input[1] == 'c'){
                                        if(breakpointCounter != MAX_BREAKPOINTS){
                                                breakpoints[breakpointCounter] = int1;
                                                breakpointCounter++;
                                                printf(GREEN "Added a breakpoint at: " RESET "0x%04x\n", int1);
                                        }else{
                                                sendWarning("No more breakpoints can be added! Remove some by doing 'bd * id', to get the ids do 'b'.");
                                        }
                                        loop();
                                }else if (input[1] == 'd'){
                                        breakpoints[int1] = 0xFFFFFFFF;
                                        loop();
                                }
                        }
                }
                // Commands with "string string Dnumber"
                if (sscanf(input, "%s %s %d", option, option, &int1)){
                        if (input[0] == 'd' && input[2] == '*'){
                                // Print memory from PC to int1
                                printMem(pds16.mem, MEMSIZE, pds16.registers[7], int1);
                                loop();
                        }
                }
                // Other Commands
                if (sscanf(input, "%s", option)){
                        if (input[0] == 'r'){
                                // Print registers
                                printRegisters(pds16.registers);
                                loop();
                        }else if (input[0] == 'm'){
                                // Print memory
                                printMem(pds16.mem, MEMSIZE, 0, MEMSIZE);
                                loop();
                        }else if (input[0] == 's'){
                                // Step
                                int opp = (pds16.mem[pds16.registers[7]]<<8)+pds16.mem[pds16.registers[7]+1];
                                decodeOp(opp);
                                pds16.registers[7]+=2;
                                loop();
                        }else if (input[0] == 'a'){
                                // Automated Mode
                                pthread_create(&tids[1], NULL, killThread, NULL);
                                pthread_create(&tids[2], NULL, run, NULL);
                                pthread_join(tids[1], NULL);
                                pthread_join(tids[2], NULL);
                                loop();
                        }else if (strcmp(input, "dump") == 0){
                                dumpMemory(pds16.mem, MEMSIZE);
                                loop();
                        }else if (input[0] == 'i'){
                                // Simulate interrupt
                                enterInterruption();
                                loop();
                        }else if (input[0] == 'b'){
                                // Show breakpoints
                                if(breakpointCounter == 0 && input[2] != '-'){
                                        printf("You have no breakpoints\n");
                                        loop();
                                }
                                printf("\nBreakpoints:\n");
                                for (int i = 0; i < ((input[2] == '-') ? MAX_BREAKPOINTS : breakpointCounter); i++){
                                        printf("ID: %d = 0x%04x\n", i, breakpoints[i]);
                                }
                                loop();
                        }
                }
        }
        sendWarning("What you entered is not a valid command.\n");
        loop();
}
