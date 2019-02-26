#include "ui.h"

static Hashtable *ht;
#define HASHTABLESIZE 151

static bool breakpointHit = false;
static bool runToBeKilled;
static bool fixedRegisters = true;
static bool fixedASM = false;
static bool clearScreenEveryCommand = true;
static bool dontPrintAnything = false;
static int interruptTime = -1;

float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

// TODO: Make these 2 functions better...
void *run()
{
    breakpointHit = false;
    struct timeval t0, t1;
    gettimeofday(&t0, 0);
    printf(GREEN "NOTICE: " RESET "Press - to end the run routine.\n");
    for (;;)
    {
        gettimeofday(&t1, 0);
        if (interruptTime != -1 && (readFromRegister(6) & 0x20) == 0)
        {
            if (timedifference_msec(t0, t1) > interruptTime)
            {
                gettimeofday(&t0, 0);
                enterInterruption();
            }
        }
        for (int i = 0; i < MAX_BREAKPOINTS - 1; i++)
        {
            if (readFromRegister(7) == breakpoints[i])
            {
                breakpointHit = true;
                printf(RED "\n\nHit breakpoint #%d at memory address: 0x%04x\n" RESET, i, breakpoints[i]);
                printf("\n\nPress ENTER to continue\n");
            }
        }
        if (runToBeKilled || breakpointHit)
        {
            runToBeKilled = false;
            pthread_exit(NULL);
        }
        int opp = (readFromRam(readFromRegister(7)) << 8) + readFromRam(readFromRegister(7) + 1);
        decodeOp(opp);
    }
}

void *killThread()
{
    char ch[255];
    while (1)
    {
        if (fgets(ch, sizeof(ch), stdin) != NULL)
        {
            ch[0] = tolower(ch[0]);
            if (ch[0] == '-')
            {
                runToBeKilled = true;
                while (runToBeKilled)
                {
                }
                pthread_exit(NULL);
            }
            else if (ch[0] == 'r')
            {
                printRegisters(pds16.mem);
            }
            else if (breakpointHit)
            {
                pthread_exit(NULL);
            }
            else
            {
                sendWarning("Running in auto mode. No commands accepted. Press - to exit or 'r' to see the registers");
            }
        }
    }
}

// Comandos da UI
void UIAuto(Input *i)
{
    (void)i;
    pthread_create(&tids[1], NULL, killThread, NULL);
    pthread_create(&tids[2], NULL, run, NULL);
    pthread_join(tids[1], NULL);
    pthread_join(tids[2], NULL);
    printf("\033[2J");
}

void UIBreakpoints(Input *i)
{
    (void)i;
    if (fixedASM == false)
    {
        printf("\nBreakpoints:\n");
        for (int i = 0; i < MAX_BREAKPOINTS; i++)
        {
            if (breakpoints[i] == (signed)0xffffffff)
            {
                printf("ID: %d = Not Set\n", i);
            }
            else
            {
                printf("ID: %d = 0x%04x\n", i, breakpoints[i]);
            }
        }
    }
    else
    {
        printf("Cannot show breakpoints while Fixed ASM option is enabled!");
    }
}

void UIBreakCreate(Input *i)
{
    int address;
    if (sscanf(i->args, "%i", &address) == 1)
    {
        breakpointManager(0, address, true);
    }
    else
    {
        printHelp("bc");
    }
}

void UIBreakDelete(Input *i)
{
    int address;
    if (sscanf(i->args, "%i", &address) == 1)
    {
        breakpointManager(0, address, true);
    }
    else
    {
        printHelp("bd");
    }
}

void UIClearScreen(Input *i)
{
    (void)i;
    printf("\033[H\033[J");
}

void UIConnect(Input *i)
{
    int address;
    if (sscanf(i->args, "%i", &address) == 1)
    {
        serverStart(address);
    }
}

void UIDecode(Input *i)
{
    int address1, address2;
    if (fixedASM == false)
    {
        if (clearScreenEveryCommand)
        {
            UIClearScreen(i);
        }
        if (sscanf(i->args, "%i %i", &address1, &address2) == 2)
        {
            address1 = address1 & 0xfffe;
            address2 = address2 & 0xfffe;
            for (int i = address1; i <= address2; i += 2)
            {
                int code = (readFromRam(i) << 8) + readFromRam(i + 1);
                printOp(code, i);
            }
        }
        else if (sscanf(i->args, "%i", &address1) == 1)
        {
            address1 = address1 & 0xfffe;
            for (int i = readFromRegister(7); i <= address1 + readFromRegister(7); i += 2)
            {
                int code = (readFromRam(i) << 8) + readFromRam(i + 1);
                printOp(code, i);
            }
        }
        else if (strlen(i->args) == 0)
        {
            int code = (readFromRam(readFromRegister(7)) << 8) + readFromRam(readFromRegister(7) + 1);
            printf("\n\n\n");
            printOp(code, readFromRegister(7));
        }
        else
        {
            printHelp("do");
            return;
        }
        printf("\nLegend: " YELLOW "Constants; " RED "Memory Addresses; " CYAN "Offsets*2; " RESET "(*)Breakpoints; <--- actual PC");
    }
    else
    {
        printf("Cannot show decoded operations while Fixed ASM is activated!");
    }
}

void UIDump(Input *i)
{
    (void)i;
    FILE *fp = fopen("memory.bin", "w+");
    if (!fp)
    {
        sendError("Could not create memory.bin!");
    }
    sendWarning("Dumping memory to memory.bin...");
    for (unsigned int i = 0; i < MEMSIZE; i++)
    {
        fprintf(fp, "%c", pds16.mem[i]);
    }
    sendWarning("Memory dumped sucessfully!");
    printf(GREEN "Dumped: " RESET "%lu bytes\n", MEMSIZE * sizeof(char));
    fclose(fp);
}

void UIExit(Input *i)
{
    (void)i;
    // Limpar Hashtable (superficialmente)
    // TODO: Limpar completamente o espaço alocado manualmente
    for (unsigned int i = 0; i < ht->size; i++)
    {
        free(ht->table[i]);
    }
    free(ht);
    exit(0);
}

void UIHelp(Input *i)
{
    if (strlen(i->args) != 0)
    {
        printHelp(i->args);
    }
    else
    {
        printHelp("all");
    }
}

void UIInterrupt(Input *i)
{
    (void)i;
    enterInterruption();
}

void UIOpenNewFile(Input *i)
{
    FILE *file;
    file = fopen(i->args, "r");
    if (!file)
    {
        printf(YELLOW "File '%s' not found!\n", i->args);
    }
    initializePDS16();
    // Clear the breakpoints
    memset(breakpoints, 0xFFFFFFFF, (MAX_BREAKPOINTS) * sizeof(int));
	fixedASM = false;
	fixedRegisters = true;
    parseHexFile(file);
    // TODO: Delete the remaining symbols and ASM

    // SHA1((unsigned char *)i->args, strlen(i->args), sha1);
    // for (unsigned int i = 0; i < strlen(i->args); i++) {
    //         printf("%x", i->args[i]);
    // }
    fclose(file);
}

void UIPatchByte(Input *i)
{
    int address, value;
    if (sscanf(i->args, "%i %i", &address, &value) == 2)
    {
        patchMemory(address, value, true);
    }
    else
    {
        printHelp("pmb");
    }
}

void UIPatchWord(Input *i)
{
    int address, value;
    if (sscanf(i->args, "%i %i", &address, &value) == 2)
    {
        patchMemory(address, value, false);
    }
    else
    {
        printHelp("pmw");
    }
}

void UIPrintMemory(Input *i)
{
    int address1, address2;
    if (fixedASM == false)
    {
        // Memory Print
        if (sscanf(i->args, "%i %i", &address1, &address2) == 2)
        {
            printMem(pds16.mem, MEMSIZE, address1, address2, pds16.nCS_Out);
        }
        else if (sscanf(i->args, "%i", &address1) == 1)
        {
            printMem(pds16.mem, MEMSIZE, readFromRegister(7), address1, pds16.nCS_Out);
        }
        else
        {
            printMem(pds16.mem, MEMSIZE, 0, MEMSIZE, pds16.nCS_Out);
        }
        printf("\n\n");
    }
    else
    {
        printf("Cannot print memory to screen while Fixed ASM is activated!");
    }
}

void UIPrintSymbols(Input *i)
{
    (void)i;
    for (int i = 0; i < symbols.addressesIn; i++)
    {
        printf("Address symbol number %i:\n", i);
        printf("Name: %s\n", symbols.addressNames[i]);
        printf("Value: 0x%04x\n\n", symbols.e_address[i]);
    }

    for (int i = 0; i < symbols.numericsIn; i++)
    {
        printf("Numeric symbol number %i:\n", i);
        printf("Name: %s\n", symbols.numericNames[i]);
        printf("Value: 0x%04x\n\n", symbols.n_address[i]);
    }
}

void UIRegisters(Input *i)
{
    (void)i;
    if (fixedASM == false && fixedRegisters == false)
    {
        printRegisters(pds16.mem);
    }
    else
    {
        printf("Cannot show registers while Fixed (ASM|Registers) option is enabled\n");
    }
}

void UISetParameters(Input *i)
{
    char parameterName[32];
    int value;
    if (sscanf(i->args, "%s %i", parameterName, &value) == 2)
    {
        if (strcmp(parameterName, "interrupttime") == 0 || strcmp(parameterName, "it") == 0)
        {
            interruptTime = value;
            if (value == -1)
            {
                printf("interruptTime has been disabled!\n");
                return;
            }
            else if (value > 10000)
            {
                printf("Maximum value is 10 seconds.\n");
                return;
            }
            printf("interruptTime has been set to %dms\n", interruptTime);
            printf("\nSet interruptTime = -1 to disable it!\n");
            return;
        }
        if (strcmp(parameterName, "showregisters") == 0 || strcmp(parameterName, "sr") == 0)
        {
            if (value > 0)
            {
                printf("Fixed registers option has been activated!\n");
                fixedRegisters = true;
                clearScreenEveryCommand = true;
            }
            else
            {
                printf("Fixed registers option has been deactivated!\n");
                fixedRegisters = false;
                if (!fixedASM)
                {
                    clearScreenEveryCommand = false;
                }
            }
            return;
        }
        if (strcmp(parameterName, "clearscreeneverycommand") == 0 || strcmp(parameterName, "csec") == 0)
        {
            if (value > 0)
            {
                printf("Clear Screen on Every Command option has been activated!\n");
                clearScreenEveryCommand = true;
            }
            else
            {
                if (fixedRegisters)
                {
                    printf("Cannot deactivate Clear Screen on Every Command option while the Fixed Registers option is still activated");
                    return;
                }
                if (fixedASM)
                {
                    printf("Cannot deactivate Clear Screen on Every Command option while the Fixed ASM option is still activated");
                    return;
                }
                printf("Clear Screen on Every Command option has been deactivated!\n");
                fixedRegisters = false;
                clearScreenEveryCommand = false;
            }
            return;
        }
        if (strcmp(parameterName, "showcodes") == 0 || strcmp(parameterName, "sc") == 0)
        {
            if (value > 0)
            {
                printf("Fixed ASM option has been activated!\n");
                fixedASM = true;
                clearScreenEveryCommand = true;
            }
            else
            {
                printf("Fixed ASM option has been deactivated!\n");
                fixedASM = false;
                if (!fixedRegisters)
                {
                    clearScreenEveryCommand = false;
                }
            }
            return;
        }
    }
    printHelp("set");
}

void UISetRegister(Input *i)
{
    int registerID, value;
    if (sscanf(i->args, "%d %i", &registerID, &value) == 2)
    {
        if (registerID > 7 || registerID < 0)
        {
            sendWarning("Tried to set a non-existant register.\n");
            return;
        }
        writeToRegister(registerID, value);
        printf("Set r%d to 0x%04x\n", registerID, value);
    }
    else
    {
        printHelp("sr");
    }
}

void UIStep(Input *i)
{
    (void)i;
    int instruction = (readFromRam(readFromRegister(7)) << 8) + readFromRam(readFromRegister(7) + 1);
    decodeOp(instruction);
}
// Fim dos comandos da UI

int hash(char *string)
{
    int hash = 0;
    for (unsigned int i = 0; i < strlen(string) - 1; i++)
    {
        hash = ((hash << 5) + hash) + string[i];
    }
    return hash % ht->size;
}

void registerCommandHandler(char *alias, void (*function_ptr)(Input *))
{
    CommandHandler *ch = malloc(sizeof(*ch));
    ch->command = strcpy(malloc(strlen(alias) + 1), alias);
    ch->function_ptr = function_ptr;
    if (ht->table[hash(alias)] != NULL)
    {
        ch->next = ht->table[hash(alias)];
    }
    ht->table[hash(alias)] = ch;
}

CommandHandler *searchCommandHandler(char *alias)
{
    CommandHandler *aux = NULL;
    for (aux = ht->table[hash(alias)]; aux != NULL; aux = aux->next)
    {
        if (memcmp(alias, aux->command, strlen(aux->command) + 1) == 0)
        {
            return aux;
        }
    }
    printHelp("all");
    return NULL;
}

void registerCommands()
{
    registerCommandHandler("a", &UIAuto);
    registerCommandHandler("auto", &UIAuto);

    registerCommandHandler("b", &UIBreakpoints);
    registerCommandHandler("breakpoints", &UIBreakpoints);

    registerCommandHandler("bc", &UIBreakCreate);
    registerCommandHandler("bd", &UIBreakDelete);

    registerCommandHandler("c", &UIClearScreen);
    registerCommandHandler("clr", &UIClearScreen);
    registerCommandHandler("cls", &UIClearScreen);
    registerCommandHandler("clear", &UIClearScreen);

    registerCommandHandler("do", &UIDecode);
    registerCommandHandler("decode", &UIDecode);

    registerCommandHandler("dump", &UIDump);

    registerCommandHandler("h", &UIHelp);
    registerCommandHandler("help", &UIHelp);

    registerCommandHandler("connect", &UIConnect);
    registerCommandHandler("conn", &UIConnect);

    registerCommandHandler("e", &UIExit);
    registerCommandHandler("exit", &UIExit);

    registerCommandHandler("i", &UIInterrupt);
    registerCommandHandler("int", &UIInterrupt);
    registerCommandHandler("interrupt", &UIInterrupt);

    registerCommandHandler("s", &UIStep);
    registerCommandHandler("si", &UIStep);
    registerCommandHandler("step", &UIStep);

    registerCommandHandler("symbols", &UIPrintSymbols);
    registerCommandHandler("sym", &UIPrintSymbols);

    registerCommandHandler("set", &UISetParameters);

    registerCommandHandler("pmb", &UIPatchByte);
    registerCommandHandler("pmw", &UIPatchWord);

    registerCommandHandler("print", &UIPrintMemory);
    registerCommandHandler("pm", &UIPrintMemory);

    registerCommandHandler("open", &UIOpenNewFile);
    registerCommandHandler("onf", &UIOpenNewFile);

    registerCommandHandler("sr", &UISetRegister);

    registerCommandHandler("r", &UIRegisters);
    registerCommandHandler("reg", &UIRegisters);
    registerCommandHandler("registers", &UIRegisters);
}

void initializeGUI()
{
    ht = malloc(sizeof *ht);
    ht->size = HASHTABLESIZE;
    ht->table = calloc(ht->size, sizeof *ht->table);
    registerCommands();

    memset(breakpoints, 0xFFFFFFFF, (MAX_BREAKPOINTS) * sizeof(int));
    memset(&interruptTime, -1, sizeof(interruptTime));

    printf("\033[H\033[J");
    fixedRegisters = true;
    clearScreenEveryCommand = true;
    fixedASM = false;
}

bool isOnBreakpointList(int address)
{
    for (int i = 0; i < MAX_BREAKPOINTS - 1; i++)
    {
        if (breakpoints[i] == address)
            return true;
    }
    return false;
}

void breakpointManager(int id, int address, bool adding)
{
    address &= 0xfffe;
    if (adding)
    {
        if (isOnBreakpointList(address))
        {
            printf(YELLOW "Warning: " RESET "The address 0x%04x is already on the breakpoint list\n", address);
        }
        for (int i = 0; i < MAX_BREAKPOINTS; i++)
        {
            if (breakpoints[i] == (signed)0xFFFFFFFF)
            {
                breakpoints[i] = address;
                printf(GREEN "Added a breakpoint at: " RESET "0x%04x\n", address);
                return;
            }
        }
        sendWarning("No more breakpoints can be added! Remove some by doing 'bd <id>', to get the ids do 'b'.");
    }
    else
    {
        if (breakpoints[id] == (signed)0xFFFFFFFF)
        {
            printf(YELLOW "Breakpoint #%d is not set!\n" RESET, id);
        }
        else
        {
            breakpoints[id] = (signed)0xFFFFFFFF;
            printf(GREEN "Breakpoint #%d deleted sucessfully!\n" RESET, id);
        }
    }
}

void fixedASMPrinting()
{
    printf("\033[1000A");
    printf("\033[1000D");
    int l = getTermHeight();
    printf("\033[%dB", 5);
    int i;
    if (readFromRegister(7) - 2 < 0)
    {
        i = 0;
    }
    else
    {
        i = readFromRegister(7) - 2;
    }
    for (; i < readFromRegister(7) + 1.5 * l; i += 2)
    {
        int code = (readFromRam(i) << 8) + readFromRam(i + 1);
        printOp(code, i);
    }
    printf("\033[1000B");
    printf("\033[2A");
    printf("Legend: " YELLOW "Constants; " RED "Memory Addresses; " CYAN "Offsets*2; " GREEN "Symbol; " RESET "(*)Breakpoints; <--- actual PC\n");
}

void fixedRegistersPrinting()
{
    int col = getTermWidth();
    int l = getTermHeight();
    char *regString;
    if ((readFromRegister(6) & 0x20) != 0)
    {
        regString = "ir";
    }
    else
    {
        regString = " r";
    }
    printf("\033[1000A");
    printf("\033[1000D");
    printf("\033[%dB", l / 2 - 8);
    printf("\033[%dC***********************\n", col - 27);
    printf("\033[%dC*                     *\n", col - 27);
    for (int i = 0; i <= 5; i++)
    {
        printf("\033[%dC*    %s%d = 0x%04X     *\n", col - 27, regString, i, readFromRegister(i) & 0xFFFF);
    }
    printf("\033[%dC*     r6 = 0x%04X     *\n", col - 27, readFromRegister(6) & 0xFFFF);
    printf("\033[%dC*     r7 = 0x%04X     *\n", col - 27, readFromRegister(7) & 0xFFFF);
    printf("\033[%dC*                     *\n", col - 27);
    printf("\033[%dC***********************\n", col - 27);
    printf("\033[1000B");
    printf("\033[1A");
}

unsigned int getTermWidth()
{
    char const *const term = getenv("TERM");
    if (!term)
    {
        fprintf(stderr, "TERM environment variable not set\n");
        return 0;
    }

    char term_buf[1024];
    switch (tgetent(term_buf, term))
    {
    case -1:
        fprintf(stderr, "tgetent() failed: terminfo database not found\n");
        return 0;
    case 0:
        fprintf(stderr, "tgetent() failed: TERM=%s not found\n", term);
        return 0;
    } // switch

    int const cols = tgetnum("co"); // number of (co)lumns
    if (cols == -1)
    {
        fprintf(stderr, "tgetnum() failed\n");
        return 0;
    }
    return cols;
}

unsigned int getTermHeight()
{
    char const *const term = getenv("TERM");
    if (!term)
    {
        fprintf(stderr, "TERM environment variable not set\n");
        return 0;
    }

    char term_buf[1024];
    switch (tgetent(term_buf, term))
    {
    case -1:
        fprintf(stderr, "tgetent() failed: terminfo database not found\n");
        return 0;
    case 0:
        fprintf(stderr, "tgetent() failed: TERM=%s not found\n", term);
        return 0;
    } // switch

    int const cols = tgetnum("li"); // number of (co)lumns
    if (cols == -1)
    {
        fprintf(stderr, "tgetnum() failed\n");
        return 0;
    }
    return cols;
}

void parseSymbolsLine(char *line)
{
    bool addressType = false;
    short int nameLen = 0;
    char name[255];
    short int value;
    int counter = 0;
    int i = 0;

    addressType = line[counter];
    counter++;
    nameLen = (line[counter] << 12) + (line[counter + 1] << 8) + (line[counter + 2] << 4) + (line[counter + 3]);
    counter += 4;
    for (i = 0; i < nameLen; i++)
    {
        name[i] = line[counter] * 16 + line[counter + 1];
        if (i + 1 <= nameLen)
            counter += 2;
    }

    name[i] = 0;
    value = (line[counter] << 12) + (line[counter + 1] << 8) + (line[counter + 2] << 4) + (line[counter + 3]);
    if (addressType)
    {
        symbols.e_address[symbols.addressesIn] = value;
        strcpy(symbols.addressNames[symbols.addressesIn], name);
        symbols.addressesIn++;
    }
    else
    {
        symbols.n_address[symbols.numericsIn] = value;
        strcpy(symbols.numericNames[symbols.numericsIn], name);
        symbols.numericsIn++;
    }
}

void parseSymbolsFile(FILE *symbols_file)
{
    printf("Started parsing symbols\n\n\n\n");
    memset(symbols.e_address, 0x00, 255 * sizeof(short int));
    memset(symbols.n_address, 0x00, 255 * sizeof(short int));
    memset(symbols.addressNames, 0x00, 255 * 255 * sizeof(char));
    memset(symbols.numericNames, 0x00, 255 * 255 * sizeof(char));
    memset(&symbols.addressesIn, 0x00, sizeof(int));
    memset(&symbols.numericsIn, 0x00, sizeof(int));
    int c, i = 0;
    char line[255];
    while ((c = fgetc(symbols_file)) != EOF)
    {
        if (c != '\n')
        {
            line[i] = getVal(c);
            i++;
        }
        if (c == '\n')
        {
            parseSymbolsLine(line);
            memset(&line, 0x00, 255 * sizeof(char));
            i = 0;
        }
    }
}

void menu()
{
    if (fixedASM && !dontPrintAnything)
    {
        fixedASMPrinting();
    }

    if (fixedRegisters && !dontPrintAnything)
    {
        fixedRegistersPrinting();
    }

    dontPrintAnything = false;

    printf("\033[%dB", 1000);
    printf("\033[%dA", 2);
    printf(LIGHT_YELLOW "\n0x%04x" RESET "> ", readFromRegister(7) & 0xffff);

    char input[255] = {0};
    char option[255] = {0};

    if (fgets(input, sizeof(input), stdin) == NULL)
    {
        printf("You closed the stdin pipe... Don't press CTRL-D\n");
        exit(-1);
    }

    if (clearScreenEveryCommand)
    {
        printf("\033[2J");
        printf("\033[1000A");
    }

    // Let's put all to lowercase!
    toLowerArray(input, sizeof(input) / sizeof(char));

    // Remember the last command!
    if (input[0] != '\n')
    {
        strcpy(lastcommand, input);
    }
    else
    {
        strcpy(input, lastcommand);
    }

    /************************ COMMANDS ***************************/
    if (sscanf(input, "%s", option) != 1)
    {
    }
    Input *newInput = malloc(sizeof *newInput);
    newInput->command = strcpy(malloc(strlen(option) + 1), option);
    newInput->args = strcpy(malloc(strlen(input + strlen(option) + 1) + 1), input + strlen(option) + 1);
    newInput->args[strlen(newInput->args) - 1] = '\0';

    CommandHandler *ch = searchCommandHandler(newInput->command);
    if (ch != NULL)
    {
        ch->function_ptr(newInput);
    }

    free(newInput->command);
    free(newInput->args);
    free(newInput);
    menu();
}

void printHelp(char *commandHelp)
{
    dontPrintAnything = true;
    printf("\033[H\033[J");
    printf("HELP:\n");
    if (strcmp(commandHelp, "all") == 0)
    {
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
    }
    else if (strcmp(commandHelp, "a") == 0)
    {
        printf("\n\tAutoRun - Makes the program run by itself\n\n\tSyntax: a");
    }
    else if (strcmp(commandHelp, "b") == 0)
    {
        printf("\n\tBreakpoint - Shows the list of available breakpoints along with their ids\n\n\tSyntax: b\n");
        printf("\n\tSee also: 'help bc' and 'help bd'\n");
        printf("\n\tNote: This command cannot be used while Fixed ASM is activated\n");
    }
    else if (strcmp(commandHelp, "bc") == 0)
    {
        printf("\n\tBreakpoint Create - Creates a breakpoint at <address>, use prefix 0x for hex addresses\n\n\tSyntax: bc <address>\n");
        printf("\n\tSee also: 'help b' and 'help bd'\n");
    }
    else if (strcmp(commandHelp, "bd") == 0)
    {
        printf("\n\tBreakpoint Delete - Deletes the breakpoint with id <id>\n\n\tSyntax: bd <id>\n");
        printf("\n\tSee also: 'help b' and 'help bc'\n");
    }
    else if (strcmp(commandHelp, "clear") == 0)
    {
        printf("\n\tClear - Clears the screen\n\n\tSyntax: clear\n");
    }
    else if (strcmp(commandHelp, "do") == 0)
    {
        printf("\n\tDecode operation - Decodes operations in memory\n\n");
        printf("\tSyntax: do <baddr> <eaddr> \t-Decodes from <baddr> to <eaddr>\n");
        printf("\tSyntax: do <eaddr> \t\t-Decodes from PC to <eaddr>\n");
        printf("\tSyntax: do <eaddr> \t\t-Decodes PC\n");
        printf("\n\tNote: This command cannot be used while Fixed ASM is activated\n");
    }
    else if (strcmp(commandHelp, "dump") == 0)
    {
        printf("\n\tDump - Creates a memory dump of the current state of the CPU RAM in 'memory.bin'\n\n\tSyntax: dump\n");
    }
    else if (strcmp(commandHelp, "e") == 0)
    {
        printf("\n\tExit - Exits the program\n\n\tSyntax: e\n");
    }
    else if (strcmp(commandHelp, "help") == 0)
    {
        printf("\n\tReally?\n");
    }
    else if (strcmp(commandHelp, "i") == 0)
    {
        printf("\n\tInterrupt - Makes the CPU enter an interrupt routine\n\n\tSyntax: i\n");
    }
    else if (strcmp(commandHelp, "mp") == 0)
    {
        printf("\n\tMemory Print - Prints the RAM memory from specified addresses\n\n");
        printf("\tSyntax: mp <baddr> <eaddr> \t-Prints the memory from <baddr> to <eaddr>\n");
        printf("\tSyntax: mp <eaddr> \t\t-Prints the memory from PC to <eaddr>\n");
        printf("\tSyntax: mp <eaddr> \t\t-Prints the memory from 0 to MEMSIZE = 0x7FFF\n");
        printf("\n\tNote: This command cannot be used while Fixed ASM is activated\n");
        printf("\tNote: Valid memory address range: 0x0000 to 0x7FFF and 0xFF40 to 0xFF7F\n");
    }
    else if (strcmp(commandHelp, "onf") == 0)
    {
        printf("\n\tOpen New File - Erases and programs the RAM with <file>\n\n\tSyntax: onf <file>\n");
    }
    else if (strcmp(commandHelp, "pmb") == 0)
    {
        printf("\n\tPatch Memory Byte - Patches Byte at <address> with <value>. Use 0x prefix for hex numbers\n\n\tSyntax: pmb <address> <value>\n");
    }
    else if (strcmp(commandHelp, "pmw") == 0)
    {
        printf("\n\tPatch Memory Word - Patches bytes at <address> and <address+1> with MSB(<value>) and LSB(<value>). Use 0x prefix for hex numbers\n\n\tSyntax: pmw <address> <value>\n");
        printf("\n\tNote: LSB(<value>) = <value> & 0xFF and MSB(<value>) = (<value>)>>8&0xFF00\n");
    }
    else if (strcmp(commandHelp, "onf") == 0)
    {
        printf("\n\tSingle Step - Increments PC and executes an operation\n\n\tSyntax: s or si\n");
    }
    else if (strcmp(commandHelp, "set") == 0)
    {
        printf("\n\tSet - Sets simulator-related options.\n\n");
        printf("\tAvailable Options:\n\n");
        printf("\tInterruptTime             - While in autorun it sets nINT to 0 every <time> miliseconds\n");
        printf("\t                          - Usage: set (interrupttime|it) <time>\n");
        printf("\t                          - Range: 0 to 10000ms\n");
        printf("\t                          - Default: -1\n");
		printf("\t                          - Value: %d\n", interruptTime);
        printf("\t                          - Deactivation: Set to -1\n\n");
        printf("\tShowRegisters             - Fixes a small 'Window' to the right with the registers\n");
        printf("\t                          - Usage: set (showregisters|sr) <value>\n");
        printf("\t                          - Range: 0 or 1\n");
        printf("\t                          - Default: 1\n");
		printf("\t                          - Value: %d\n", fixedRegisters);
        printf("\t                          - Deactivation: Set to 0\n\n");
        printf("\tClearScreenEveryCommand   - Everytime a command is executed it cleans the screen\n");
        printf("\t                          - Usage: set (clearscreeneverycommand|csec) <value>\n");
        printf("\t                          - Range: 0 or 1\n");
        printf("\t                          - Default: 1\n");
		printf("\t                          - Value: %d\n", clearScreenEveryCommand);
        printf("\t                          - Deactivation: Set to 0\n\n");
        printf("\tShowCodes                 - Prints, on the middle of the screen, an attempt of the reconstruction of the ASM\n");
        printf("\t                          - Usage: set (showcodes|sc) <value>\n");
        printf("\t                          - Range: 0 or 1\n");
        printf("\t                          - Default: 0\n");
		printf("\t                          - Value: %d\n", fixedASM);
        printf("\t                          - Deactivation: Set to 0\n\n");
    }
    else if (strcmp(commandHelp, "sr") == 0)
    {
        printf("\n\tSet Register - Sets register r<id> to <value>\n\n\tSyntax: sr <id> <value>\n");
    }
    else if (strcmp(commandHelp, "r") == 0)
    {
        printf("\n\tPrints registers to the screen once\n\n\tSyntax: r\n");
        printf("\n\tNote: This command cannot be used while Fixed ASM is activated\n");
    }
    else
    {
        printf("Couldn't understand '%s'!\n", commandHelp);
    }
}
