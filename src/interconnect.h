// logging.c
void sendWarning(char c[]);
void sendError(char c[]);
void printMem(unsigned char * mem, int memSize, int beginning, int end, unsigned char nCS_Out);
void printRegisters(unsigned char * mem);
void printPSW(short int PSW);
char * toLowerArray(char * array, int sizeArray);
void printOp(int code, int memoryAddress);

// cpu.c
short int readFromRegister(int registerID);
void dumpMemory(unsigned char * memory, long unsigned int memSize);
void writeToRegister(int registerID, short int value);
void enterInterruption();
void exitInterruption();
void erasePDS(unsigned char * mem);
void patchMemory(int address, int value, bool byte);
void programRam(unsigned char * mem, char * Line, int addressToWrite);
void writeToRam(short int value, int address);
short int readFromRam(int address);
int getVal(char c);
int getAddressFromLine(char * Line);
int parseHexFile(unsigned char * mem, FILE *fileopened);
void initializePDS16();
void *run();
void *killThread();
int decodeOp(unsigned int code);

// main.c
void fixedRegistersPrinting();
bool isOnBreakpointList(int address);
void breakpointManager(int id, int address, bool adding);
int main(int argc, char const *argv[]);
void menu();

// microcode.c
void ldi(int code);
void ldih(int code);
void ld(int code);
void st(int code);
void add(int code);
void adc(int code);
void sub(int code);
void sbb(int code);
void anl(int code);
void orl(int code);
void xrl(int code);
void not(int code);
void shl(int code);
void shr(int code);
void rr(int code);
void jz(int code);
void jnz(int code);
void jc(int code);
void jnc(int code);
void jmp(int code);
void jmpl(int code);
void iret();

// gui.c
unsigned getTermWidth();
unsigned getTermHeight();
void fixedASMPrinting();
void fixedRegistersPrinting();
void printHelp(char * commandHelp);
