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
        printf("\033[H\033[J");
        fixedRegisters = true;
        clearScreenEveryCommand = true;
        fixedASM = false;
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

// TODO: Criar suporte para novos ficheiros DONE
// TODO: Criar um novo menu KINDA
// TODO: Disassemble DONE
// TODO: Dump memory to a file DONE
// TODO: Separar e melhor hierarquia dos ficheiros KINDA
// TODO: Melhorar a implementação do ISA KINDA
// TODO: Patch Memory DONE
