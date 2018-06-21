#include "main.h"

void breakpointManager(int id, int address, bool adding){
        if(adding){
                if((id+1) != MAX_BREAKPOINTS){
                        breakpoints[id] = address;
                        breakpointCounter++;
                        printf(GREEN "Added a breakpoint at: " RESET "0x%04x\n", address);
                        }else{
                        sendWarning("No more breakpoints can be added! Remove some by doing 'bd <id>', to get the ids do 'b'.");
                }
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

void menu(){
        printf("\n0x%04x>", readFromRegister(7));
        char input[255] = {0};
        char option[10] = {0};
        int int1 = 0xFFFFFFFF;
        int int2 = 0xFFFFFFFF;
        if (fgets(input, sizeof(input), stdin) != NULL)
        // Let's put all to lowercase!
        toLowerArray(input, sizeof(input)/sizeof(char));
        // Remember the last command!
        if(input[0] != '\n'){
                strcpy(lastcommand, input);
        }else{
                strcpy(input, lastcommand);
        }

        // For some reason the program SEGFAULTS if this isn't here
        // if(sscanf(input, "%s", option) != 1){
        // }
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
        if (input[0] == 'e') exit(0);
        if (option[0] == 'i'){
                enterInterruption();
                menu();
        }
        if (option[0] == 'r'){
                printRegisters(pds16.registers);
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
                        breakpointManager(breakpointCounter, int1, true);
                        menu();
                }
        }
        if (strcmp(option, "bd") == 0){
                if(sscanf(input, "%s %i", option, &int1) == 2){
                        breakpointManager(int1, 0, false);
                        menu();
                }
        }
        if (strcmp(option, "dump") == 0){
                dumpMemory(pds16.mem, MEMSIZE);
                menu();
        }
        if (strcmp(option, "pm") == 0){
                if(sscanf(input, "%s %i %i", option, &int1, &int2) == 3){
                        printMem(pds16.mem, MEMSIZE, int1, int2);
                }else if (sscanf(input, "%s %i", option, &int1) == 2){
                        printMem(pds16.mem, MEMSIZE, readFromRegister(7), int1);
                }else{
                        printMem(pds16.mem, MEMSIZE, 0, MEMSIZE);
                }
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
        if ((strcmp(option, "s") && strcmp(option, "si")) == 0){
                // Single Step
                int instruction = (pds16.mem[readFromRegister(7)]<<8)+pds16.mem[readFromRegister(7)+1];
                decodeOp(instruction);
                writeToRegister(7, readFromRegister(7)+2);
                menu();
        }
        printf("\nThat command is unknown!\n");
        menu();
}

// TODO: Criar suporte para novos ficheiros
// TODO: Criar um novo menu KINDA
// TODO: Disassemble
// TODO: Dump memory to a file DONE
// TODO: Separar e melhor hierarquia dos ficheiros KINDA
// TODO: Melhorar a implementação do ISA
