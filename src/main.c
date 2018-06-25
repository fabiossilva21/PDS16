#include "main.h"

bool isOnBreakpointList(int address){
        for (int i = 0; i < MAX_BREAKPOINTS-1; i++){
                if (breakpoints[i] == address)
                        return true;
        }
        return false;
}

void breakpointManager(int id, int address, bool adding){
        address &= 0xfffe;
        if(adding){
                if (isOnBreakpointList(address)){
                        printf(YELLOW "Warning: " RESET "The address 0x%04x is already on the breakpoint list\n", address);
                        menu();
                }
                for(int i = 0; i < MAX_BREAKPOINTS-1; i++){
                        if(breakpoints[i] == 0xFFFFFFFF){
                                breakpoints[i] = address;
                                printf(GREEN "Added a breakpoint at: " RESET "0x%04x\n", address);
                                menu();
                        }
                }
                sendWarning("No more breakpoints can be added! Remove some by doing 'bd <id>', to get the ids do 'b'.");
        }else{
                if(breakpoints[id] == 0xFFFFFFFF){
                        printf(YELLOW"Breakpoint #%d is not set!\n"RESET, id);
                }else{
                        breakpoints[id] = 0xFFFFFFFF;
                        printf(GREEN "Breakpoint #%d deleted sucessfully!\n" RESET, id);
                }
        }
}

int main(int argc, char const *argv[]) {
        advancedPrinting = false;
        FILE *file;
        file = fopen(argv[1], "r");
        if(!file){
                printf(RED "Improper calling!\n\n" RESET);
                printf("Syntax of the programm:\n");
                printf("./PDS16 <hex_file>\n\n");
                exit(1);
        }
        initializePDS16();
        parseHexFile(pds16.mem, file);
        fclose(file);
        menu();
        return 0;
}

void fixedRegistersPrinting(){
        int col = getTermWidth();
        int lines = getTermHeight();
        printf("\033[1000A");
        printf("\033[%dB", lines/2-6);
        printf("\033[%dC************************\n", col-27);
        printf("\033[%dC*                      *\n", col-27);
        printf("\033[%dC*    (i)r0 = 0x%04X    *\n", col-27, readFromRegister(0)&0xFFFF);
        printf("\033[%dC*    (i)r1 = 0x%04X    *\n", col-27, readFromRegister(1)&0xFFFF);
        printf("\033[%dC*    (i)r2 = 0x%04X    *\n", col-27, readFromRegister(2)&0xFFFF);
        printf("\033[%dC*    (i)r3 = 0x%04X    *\n", col-27, readFromRegister(3)&0xFFFF);
        printf("\033[%dC*    (i)r4 = 0x%04X    *\n", col-27, readFromRegister(4)&0xFFFF);
        printf("\033[%dC*    (i)r5 = 0x%04X    *\n", col-27, readFromRegister(5)&0xFFFF);
        printf("\033[%dC*       r6 = 0x%04X    *\n", col-27, readFromRegister(6)&0xFFFF);
        printf("\033[%dC*       r7 = 0x%04X    *\n", col-27, readFromRegister(7)&0xFFFF);
        printf("\033[%dC*                      *\n", col-27);
        printf("\033[%dC************************\n", col-27);
        printf("\033[1000B");
        printf("\033[1A");
}

void menu(){
        if (advancedPrinting == true){
                fixedRegistersPrinting();
        }
        printf(LIGHT_YELLOW "\n0x%04x" RESET "> ", readFromRegister(7));
        char input[255] = {0};
        char option[255] = {0};
        int int1 = 0xFFFFFFFF;
        int int2 = 0xFFFFFFFF;
        if (fgets(input, sizeof(input), stdin) == NULL){
                printf("You closed the stdin pipe... Don't press CTRL-D\n");
                exit(-1);
        }
        if (advancedPrinting){
                printf("\033[2J");
        }
        // Let's put all to lowercase!
        toLowerArray(input, sizeof(input)/sizeof(char));
        // Remember the last command!
        if(input[0] != '\n'){
                strcpy(lastcommand, input);
        }else{
                strcpy(input, lastcommand);
        }

        /************************ COMMANDS ***************************/
        if(sscanf(input, "%s", option) != 1){
        }
        if (option[0] == 'a') {
                pthread_create(&tids[1], NULL, killThread, NULL);
                pthread_create(&tids[2], NULL, run, NULL);
                pthread_join(tids[1], NULL);
                pthread_join(tids[2], NULL);
                menu();
        }
        if (option[0] == 'e') exit(0);
        if (option[0] == 'i'){
                enterInterruption();
                menu();
        }
        if (option[0] == 'r'){
                printRegisters(pds16.mem);
                menu();
        }
        if (strcmp(option, "b") == 0){
                // Show breakpoints
                printf("\nBreakpoints:\n");
                for (int i = 0; i < MAX_BREAKPOINTS-1; i++){
                        if(breakpoints[i] == 0xffffffff){
                                printf("ID: %d = Not Set\n",i);
                        }else{
                                printf("ID: %d = 0x%04x\n", i, breakpoints[i]);
                        }
                }
                menu();
        }
        if (strcmp(option, "bc") == 0){
                if(sscanf(input, "%s %i", option, &int1) == 2){
                        breakpointManager(0, int1, true);
                        menu();
                }
        }
        if (strcmp(option, "bd") == 0){
                if(sscanf(input, "%s %i", option, &int1) == 2){
                        breakpointManager(int1, 0, false);
                        menu();
                }
        }
        if (strcmp(option, "clear") == 0){
                printf("\033[H\033[J");
                menu();
        }
        if (strcmp(option, "do") == 0){
                printf("\n");
                if (sscanf(input, "%s %i %i", option, &int1, &int2) == 3){
                        int1 = int1 & 0xfffe;
                        for(int i = int1; i <= int2; i+=2){
                                int code = (readFromRam(i)<<8)+readFromRam(i+1);
                                printOp(code, i);
                        }
                }else if (sscanf(input, "%s %i", option, &int1) == 2){
                        int1 = int1 & 0xfffe;
                        for(int i = readFromRegister(7); i <= int1+readFromRegister(7); i+=2){
                                int code = (readFromRam(i)<<8)+readFromRam(i+1);
                                printOp(code, i);
                        }
                }else {
                        int code = (readFromRam(readFromRegister(7))<<8)+readFromRam(readFromRegister(7)+1);
                        printOp(code, readFromRegister(7));
                }
                printf("\nLegend: " YELLOW "Constants; " RED "Memory Addresses; " CYAN "Offsets*2\n" RESET);
                menu();
        }
        if (strcmp(option, "dump") == 0){
                dumpMemory(pds16.mem, MEMSIZE);
                menu();
        }
        if (strcmp(option, "mp") == 0){
                // Memory Print
                if(sscanf(input, "%s %i %i", option, &int1, &int2) == 3){
                        printMem(pds16.mem, MEMSIZE, int1, int2);
                }else if (sscanf(input, "%s %i", option, &int1) == 2){
                        printMem(pds16.mem, MEMSIZE, readFromRegister(7), int1);
                }else{
                        printMem(pds16.mem, MEMSIZE, 0, MEMSIZE);
                }
                menu();
        }
        if (strcmp(option, "onf") == 0){
                if(sscanf(input, "%s %s", option, option) == 2){
                        FILE *file;
                        file = fopen(option, "r");
                        if(!file){
                                printf(YELLOW "File '%s' not found!\n", option);
                                menu();
                        }
                        erasePDS(pds16.mem);
                        parseHexFile(pds16.mem, file);
                        fclose(file);
                        menu();
                }else{
                        printf("Please specify the name of the file: ");
                        if (fgets(input, sizeof(input), stdin) != NULL){
                                if(sscanf(input, "%s", option) == 1){
                                        FILE *file;
                                        file = fopen(option, "r");
                                        if(!file){
                                                printf(YELLOW "File '%s' not found!\n", option);
                                                menu();
                                        }
                                        erasePDS(pds16.mem);
                                        parseHexFile(pds16.mem, file);
                                        fclose(file);
                                        menu();
                                }
                        }
                }
        }
        if (strcmp(option, "pmb") == 0){
                // Patch memory byte
                if (sscanf(input, "%s %i %i", option, &int1, &int2) == 3){
                        patchMemory(int1, int2, true);
                }
                menu();
        }
        if (strcmp(option, "pmw") == 0){
                // Patch memory word
                if (sscanf(input, "%s %i %i", option, &int1, &int2) == 3){
                        patchMemory(int1, int2, false);
                }
                menu();
        }
        if ((strcmp(option, "s") && strcmp(option, "si")) == 0){
                // Single Step
                int instruction = (readFromRam(readFromRegister(7))<<8)+readFromRam(readFromRegister(7)+1);
                decodeOp(instruction);
                menu();
        }
        if (strcmp(option, "set") == 0){
                if (sscanf(input, "%s %s %i", option, option, &int1) == 3){
                        if(strcmp(option, "interrupttime") == 0){
                                interruptTime = int1;
                                if (int1 == -1){
                                        printf("interruptTime has been disabled!\n");
                                        menu();
                                }
                                printf("interruptTime has been set to %dms\n", interruptTime);
                                printf("\nSet interruptTime = -1 to disable it!\n");
                                menu();
                        }
                        if(strcmp(option, "showregisters") == 0 || strcmp(option, "sr") == 0 ){
                                if (int1 >= 0){
                                        printf("Advanced Printing has been activated!\n");
                                        advancedPrinting = true;
                                }else{
                                        printf("Advanced Printing has been deactivated!\n");
                                        advancedPrinting = false;
                                }
                                menu();
                        }
                }
                sendWarning("Wrong usage!");
                menu();
        }
        if (strcmp(option, "sr") == 0){
                if (sscanf(input, "%s %c%d %i", option, option, &int1, &int2) == 4) {
                       if(int1 > 7 || int1 < 0){
                               sendWarning("Tried to set a non-existant register.\n");
                               menu();
                       }
                       writeToRegister(int1, int2);
                       printf("Set r%d to 0x%04x\n", int1, int2);
               }else{
                       printf("Syntax: sr r<id> <value>        <- Sets register #<id> to <value>");
               }
                menu();
        }
        printf("\nThe command '%s' is unknown!\n", option);
        menu();
}

// TODO: Criar suporte para novos ficheiros DONE
// TODO: Criar um novo menu KINDA
// TODO: Disassemble DONE
// TODO: Dump memory to a file DONE
// TODO: Separar e melhor hierarquia dos ficheiros KINDA
// TODO: Melhorar a implementação do ISA KINDA
// TODO: Patch Memory DONE
