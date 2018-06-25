#include "gui.h"

void fixedASMPrinting(){
        printf("\033[1000A");
        printf("\033[1000D");
        int l = getTermHeight();
        printf("\033[%dB", lines/2-14);
        int i;
        if (readFromRegister(7)-2 < 0){
                i = 0;
        }else{
                i = readFromRegister(7)-2;
        }
        for(; i < readFromRegister(7)+1.5*l; i+=2){
                int code = (readFromRam(i)<<8)+readFromRam(i+1);
                printOp(code, i);
        }
        printf("\033[1000B");
        printf("\033[2A");
        printf("Legend: " YELLOW "Constants; " RED "Memory Addresses; " CYAN "Offsets*2; " RESET "(*)Breakpoints; <--- actual PC\n");
}

void fixedRegistersPrinting(){
        int col = getTermWidth();
        int l = getTermHeight();
        char * regString;
        if ((readFromRegister(6) & 0x20) != 0){
                regString = "ir";
        }else{
                regString = " r";
        }
        printf("\033[1000A");
        printf("\033[1000D");
        printf("\033[%dB", l/2-8);
        printf("\033[%dC***********************\n", col-27);
        printf("\033[%dC*                     *\n", col-27);
        for (int i = 0; i <= 5; i++){
                printf("\033[%dC*    %s%d = 0x%04X     *\n", col-27, regString, i, readFromRegister(i)&0xFFFF);
        }
        printf("\033[%dC*     r6 = 0x%04X     *\n", col-27, readFromRegister(6)&0xFFFF);
        printf("\033[%dC*     r7 = 0x%04X     *\n", col-27, readFromRegister(7)&0xFFFF);
        printf("\033[%dC*                     *\n", col-27);
        printf("\033[%dC***********************\n", col-27);
        printf("\033[1000B");
        printf("\033[1A");
}

unsigned int getTermWidth(){
        char const *const term = getenv( "TERM" );
        if (!term) {
                fprintf(stderr, "TERM environment variable not set\n");
                return 0;
        }

        char term_buf[1024];
        switch (tgetent(term_buf, term)) {
                case -1:
                fprintf( stderr, "tgetent() failed: terminfo database not found\n" );
                return 0;
                case 0:
                fprintf( stderr, "tgetent() failed: TERM=%s not found\n", term );
                return 0;
        } // switch

        int const cols = tgetnum("co");  // number of (co)lumns
        if (cols == -1) {
                fprintf( stderr, "tgetnum() failed\n" );
                return 0;
        }
        return cols;
}

unsigned int getTermHeight(){
        char const *const term = getenv( "TERM" );
        if (!term) {
                fprintf(stderr, "TERM environment variable not set\n");
                return 0;
        }

        char term_buf[1024];
        switch (tgetent(term_buf, term)) {
                case -1:
                fprintf( stderr, "tgetent() failed: terminfo database not found\n" );
                return 0;
                case 0:
                fprintf( stderr, "tgetent() failed: TERM=%s not found\n", term );
                return 0;
        } // switch

        int const cols = tgetnum("li");  // number of (co)lumns
        if (cols == -1) {
                fprintf( stderr, "tgetnum() failed\n" );
                return 0;
        }
        return cols;
}

void menu(){
        if (fixedASM && !dontPrintAnything){
                fixedASMPrinting();
        }
        if (fixedRegisters && !dontPrintAnything){
                fixedRegistersPrinting();
        }
        dontPrintAnything = false;
        printf("\033[%dB", 1000);
        printf("\033[%dA", 2);
        printf(LIGHT_YELLOW "\n0x%04x" RESET "> ", readFromRegister(7));
        char input[255] = {0};
        char option[255] = {0};
        int int1 = 0xFFFFFFFF;
        int int2 = 0xFFFFFFFF;
        if (fgets(input, sizeof(input), stdin) == NULL){
                printf("You closed the stdin pipe... Don't press CTRL-D\n");
                exit(-1);
        }
        if (clearScreenEveryCommand){
                printf("\033[2J");
                printf("\033[1000A");
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
                printf("\033[2J");
                menu();
        }
        if (option[0] == 'e') exit(0);
        if (option[0] == 'i'){
                enterInterruption();
                menu();
        }
        if (option[0] == 'r'){
                if (fixedASM == false && fixedRegisters == false){
                        printRegisters(pds16.mem);
                        menu();
                }else{
                        printf("Cannot show registers while Fixed (ASM|Registers) option is enabled\n");
                        menu();
                }
        }
        if (strcmp(option, "b") == 0){
                // Show breakpoints
                if (fixedASM == false){
                        printf("\nBreakpoints:\n");
                        for (int i = 0; i < MAX_BREAKPOINTS-1; i++){
                                if(breakpoints[i] == 0xffffffff){
                                        printf("ID: %d = Not Set\n",i);
                                }else{
                                        printf("ID: %d = 0x%04x\n", i, breakpoints[i]);
                                }
                        }
                        menu();
                }else{
                        printf("Cannot show breakpoints while Fixed ASM option is enabled!");
                        menu();
                }
        }
        if (strcmp(option, "bc") == 0){
                if(sscanf(input, "%s %i", option, &int1) == 2){
                        breakpointManager(0, int1, true);
                        menu();
                }else{
                        printHelp("bc");
                }
        }
        if (strcmp(option, "bd") == 0){
                if(sscanf(input, "%s %i", option, &int1) == 2){
                        breakpointManager(int1, 0, false);
                        menu();
                }else{
                        printHelp("bd");
                }
        }
        if (strcmp(option, "clear") == 0){
                printf("\033[H\033[J");
                menu();
        }
        if (strcmp(option, "do") == 0){
                if (fixedASM == false){
                        if (clearScreenEveryCommand){
                                printf("\033[%dA", 1000);
                                printf("\033[%dB", lines/2-18);
                        }
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
                                for (int i = readFromRegister(7); i <= readFromRegister(7)+2; i+=2){
                                        int code = (readFromRam(readFromRegister(7))<<8)+readFromRam(readFromRegister(7)+1);
                                        printOp(code, readFromRegister(7));
                                }
                        }
                        if (clearScreenEveryCommand){
                                printf("\033[%dA", 2);
                        }
                        printf("\nLegend: " YELLOW "Constants; " RED "Memory Addresses; " CYAN "Offsets*2; " RESET "(*)Breakpoints; <--- actual PC");
                        menu();
                }else {
                        printf("Cannot show decoded operations while Fixed ASM is activated!");
                        menu();
                }
        }
        if (strcmp(option, "dump") == 0){
                dumpMemory(pds16.mem, MEMSIZE);
                menu();
        }
        if (strcmp(option, "help") == 0){
                if(sscanf(input, "%s %s", option, option) == 2){
                        printHelp(option);
                }else{
                        printHelp("all");
                }
                menu();
        }
        if (strcmp(option, "mp") == 0){
                if (fixedASM == false){
                        // Memory Print
                        if(sscanf(input, "%s %i %i", option, &int1, &int2) == 3){
                                printMem(pds16.mem, MEMSIZE, int1, int2, pds16.nCS_Out);
                        }else if (sscanf(input, "%s %i", option, &int1) == 2){
                                printMem(pds16.mem, MEMSIZE, readFromRegister(7), int1, pds16.nCS_Out);
                        }else{
                                printMem(pds16.mem, MEMSIZE, 0, MEMSIZE, pds16.nCS_Out);
                        }
                        menu();
                }else{
                        printf("Cannot print memory to screen while Fixed ASM is activated!");
                        menu();
                }
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
                }else{
                        printHelp("pmb");
                }
                menu();
        }
        if (strcmp(option, "pmw") == 0){
                // Patch memory word
                if (sscanf(input, "%s %i %i", option, &int1, &int2) == 3){
                        patchMemory(int1, int2, false);
                }else{
                        printHelp("pmw");
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
                        if(strcmp(option, "interrupttime") == 0 || strcmp(option, "it") == 0){
                                interruptTime = int1;
                                if (int1 == -1){
                                        printf("interruptTime has been disabled!\n");
                                        menu();
                                } else if (int1 > 10000){
                                        printf("Maximum value is 10 seconds.\n");
                                        menu();
                                }
                                printf("interruptTime has been set to %dms\n", interruptTime);
                                printf("\nSet interruptTime = -1 to disable it!\n");
                                menu();
                        }
                        if(strcmp(option, "showregisters") == 0 || strcmp(option, "sr") == 0 ){
                                if (int1 > 0){
                                        printf("Fixed registers option has been activated!\n");
                                        fixedRegisters = true;
                                        clearScreenEveryCommand = true;
                                }else{
                                        printf("Fixed registers option has been deactivated!\n");
                                        fixedRegisters = false;
                                        if (!fixedASM){
                                                clearScreenEveryCommand = false;
                                        }
                                }
                                menu();
                        }
                        if(strcmp(option, "clearscreeneverycommand") == 0 || strcmp(option, "csec") == 0 ){
                                if (int1 > 0){
                                        printf("Clear Screen on Every Command option has been activated!\n");
                                        clearScreenEveryCommand = true;
                                }else {
                                        if (fixedRegisters){
                                                printf("Cannot deactivate Clear Screen on Every Command option while the Fixed Registers option is still activated");
                                                menu();
                                        }
                                        if (fixedASM){
                                                printf("Cannot deactivate Clear Screen on Every Command option while the Fixed ASM option is still activated");
                                                menu();
                                        }
                                        printf("Clear Screen on Every Command option has been deactivated!\n");
                                        fixedRegisters = false;
                                        clearScreenEveryCommand = false;
                                }
                                menu();
                        }
                        if(strcmp(option, "showcodes") == 0 || strcmp(option, "sc") == 0 ){
                                if (int1 > 0){
                                        printf("Fixed ASM option has been activated!\n");
                                        fixedASM = true;
                                        clearScreenEveryCommand = true;
                                }else{
                                        printf("Fixed ASM option has been deactivated!\n");
                                        fixedASM = false;
                                        if (!fixedRegisters){
                                                clearScreenEveryCommand = false;
                                        }
                                }
                                menu();
                        }
                }
                printHelp("set");
                menu();
        }
        if (strcmp(option, "sr") == 0){
                if (sscanf(input, "%s %d %i", option, &int1, &int2) == 4) {
                       if(int1 > 7 || int1 < 0){
                               sendWarning("Tried to set a non-existant register.\n");
                               menu();
                       }
                       writeToRegister(int1, int2);
                       printf("Set r%d to 0x%04x\n", int1, int2);
               }else{
                       printHelp("sr");
               }
                menu();
        }
        printHelp("all");
        printf("\nThe command '%s' is unknown!\n", option);
        menu();
}

void printHelp(char * commandHelp){
        dontPrintAnything = true;
        printf("\033[H\033[J");
        printf("HELP:\n");
        if (strcmp(commandHelp, "all") == 0){
                printf("Available commands:\n\n");
                printf("\t(a)\t\tAutorun - Makes the program run by itself\n");
                printf("\t(b)\t\tBreakpoint - Shows the list of available breakpoints along with their ids\n");
                printf("\t(bc <address>)\tBreakpoint Create - Creates a breakpoint at <address>, use prefix 0x for hex addresses\n");
                printf("\t(bd <id>)\tBreakpoint Delete - Deletes the breakpoint with id <id>\n");
                printf("\t(clear)\t\tClear - Clears the screen\n");
                printf("\t(do)\t\tDecode Operation - type 'help do' for syntax and more help\n");
                printf("\t(dump)\t\tDump - Creates a memory dump of the current state of the CPU RAM in 'memory.bin'\n");
                printf("\t(e)\t\tExit - Exits the program\n");
                printf("\t(help [option])\tHelp - This help screen. [option] is not required, but if present prints help for [option] command\n");
                printf("\t(i)\t\tInterrupt - Makes the CPU enter an interrupt routine\n");
                printf("\t(mp)\t\tMemory Print - type 'help mp' for syntax and more help\n");
                printf("\t(onf <file>)\tOpen New File - Erases and programs the RAM with <file>\n");
                printf("\t(pmb <a> <v>)\tPatch Memory Byte - Patches Byte at <a> with <v>. Use 0x prefix for hex numbers\n");
                printf("\t(pmw <a> <v>)\tPatch Memory word - Patches bytes at <a> and <a+1> with MSB(<v>) and LSB(<v>). Use 0x prefix for hex numbers\n");
                printf("\t(s || si)\tSingle Step - Increments PC and executes an operation\n");
                printf("\t(set)\t\tSet - Sets simulator-related options. Check 'help set' to see syntax and available options\n");
                printf("\t(sr <r> <v>)\tSet Register - Sets register r<r> to <v>\n");
                printf("\t(r)\t\tRegisters - Prints registers to the screen once\n");
        }else if (strcmp(commandHelp, "a") == 0){
                printf("\n\tAutoRun - Makes the program run by itself\n\n\tSyntax: a");
        }else if (strcmp(commandHelp, "b") == 0){
                printf("\n\tBreakpoint - Shows the list of available breakpoints along with their ids\n\n\tSyntax: b\n");
                printf("\n\tSee also: 'help bc' and 'help bd'\n");
                printf("\n\tNote: This command cannot be used while Fixed ASM is activated\n");
        }else if (strcmp(commandHelp, "bc") == 0){
                printf("\n\tBreakpoint Create - Creates a breakpoint at <address>, use prefix 0x for hex addresses\n\n\tSyntax: bc <address>\n");
                printf("\n\tSee also: 'help b' and 'help bd'\n");
        }else if (strcmp(commandHelp, "bd") == 0){
                printf("\n\tBreakpoint Delete - Deletes the breakpoint with id <id>\n\n\tSyntax: bd <id>\n");
                printf("\n\tSee also: 'help b' and 'help bc'\n");
        }else if (strcmp(commandHelp, "clear") == 0){
                printf("\n\tClear - Clears the screen\n\n\tSyntax: clear\n");
        }else if (strcmp(commandHelp, "do") == 0){
                printf("\n\tDecode operation - Decodes operations in memory\n\n");
                printf("\tSyntax: do <baddr> <eaddr> \t-Decodes from <baddr> to <eaddr>\n");
                printf("\tSyntax: do <eaddr> \t\t-Decodes from PC to <eaddr>\n");
                printf("\tSyntax: do <eaddr> \t\t-Decodes PC\n");
                printf("\n\tNote: This command cannot be used while Fixed ASM is activated\n");
        }else if (strcmp(commandHelp, "dump") == 0){
                printf("\n\tDump - Creates a memory dump of the current state of the CPU RAM in 'memory.bin'\n\n\tSyntax: dump\n");
        }else if (strcmp(commandHelp, "e") == 0){
                printf("\n\tExit - Exits the program\n\n\tSyntax: e\n");
        }else if (strcmp(commandHelp, "help") == 0){
                printf("\n\tReally?\n");
        }else if (strcmp(commandHelp, "i") == 0){
                printf("\n\tInterrupt - Makes the CPU enter an interrupt routine\n\n\tSyntax: i\n");
        }else if (strcmp(commandHelp, "mp") == 0){
                printf("\n\tMemory Print - Prints the RAM memory from specified addresses\n\n");
                printf("\tSyntax: mp <baddr> <eaddr> \t-Prints the memory from <baddr> to <eaddr>\n");
                printf("\tSyntax: mp <eaddr> \t\t-Prints the memory from PC to <eaddr>\n");
                printf("\tSyntax: mp <eaddr> \t\t-Prints the memory from 0 to MEMSIZE = 0x7FFF\n");
                printf("\n\tNote: This command cannot be used while Fixed ASM is activated\n");
                printf("\tNote: Valid memory address range: 0x0000 to 0x7FFF and 0xFF40 to 0xFF7F\n");
        }else if (strcmp(commandHelp, "onf") == 0){
                printf("\n\tOpen New File - Erases and programs the RAM with <file>\n\n\tSyntax: onf <file>\n");
        }else if (strcmp(commandHelp, "pmb") == 0){
                printf("\n\tPatch Memory Byte - Patches Byte at <address> with <value>. Use 0x prefix for hex numbers\n\n\tSyntax: pmb <address> <value>\n");
        }else if (strcmp(commandHelp, "pmw") == 0){
                printf("\n\tPatch Memory Word - Patches bytes at <address> and <address+1> with MSB(<value>) and LSB(<value>). Use 0x prefix for hex numbers\n\n\tSyntax: pmw <address> <value>\n");
                printf("\n\tNote: LSB(<value>) = <value> & 0xFF and MSB(<value>) = (<value>)>>8&0xFF00\n");
        }else if (strcmp(commandHelp, "onf") == 0){
                printf("\n\tSingle Step - Increments PC and executes an operation\n\n\tSyntax: s or si\n");
        }else if (strcmp(commandHelp, "set") == 0){
                printf("\n\tSet - Sets simulator-related options.\n\n");
                printf("\tAvailable Options:\n");
                printf("\tInterruptTime - While in autorun it simulates nINT to 0. In miliseconds\n");
                printf("\t              - Usage: set (interrupttime|it) <time>");
                printf("\t              - Range: 0 to 10000ms\n");
                printf("\t              - Default: -1");
                printf("\t              - Deactivation: Set to -1\n\n");
                printf("\tShowRegisters - Fixes a small 'Window' to the right with the registers\n");
                printf("\t              - Usage: set (showregisters|sr) <value>");
                printf("\t              - Range: 0 or 1\n");
                printf("\t              - Default: 1");
                printf("\t              - Deactivation: Set to 0\n\n");
                printf("\tClearScreenEveryCommand - Everytime a command is executed it cleans the screen\n");
                printf("\t                        - Usage: set (clearscreeneverycommand|csec) <value>");
                printf("\t                        - Range: 0 or 1\n");
                printf("\t                        - Default: 1");
                printf("\t                        - Deactivation: Set to 0\n\n");
                printf("\tShowCodes - Prints, on the middle of the screen, an attempt of the reconstruction of the ASM\n");
                printf("\t                        - Usage: set (showcodes|sc) <value>");
                printf("\t                        - Range: 0 or 1\n");
                printf("\t                        - Default: -0");
                printf("\t                        - Deactivation: Set to 0\n\n");

        }else if (strcmp(commandHelp, "sr") == 0){
                printf("\n\tSet Register - Sets register r<id> to <value>\n\n\tSyntax: sr <id> <value>\n");
        }else if (strcmp(commandHelp, "r") == 0){
                printf("\n\tPrints registers to the screen once\n\n\tSyntax: r\n");
                printf("\n\tNote: This command cannot be used while Fixed ASM is activated\n");
        }else{
                printf("Couldn't understand '%s'!\n", commandHelp);
        }
        // printf("\t(r)\t\tRegisters - Prints registers to the screen once\n");
}
