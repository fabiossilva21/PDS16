#ifndef _INTERCONNECT_H_
#define _INTERCONNECT_H_

// TODO: Get rid of this entire file

// logging.c
void sendWarning(char c[]);
void sendError(char c[]);
void printRegisters();
void printMem(unsigned char * mem, int memSize, int beginning, int end, unsigned char nCS_Out);
void printPSW(short int PSW);
char * toLowerArray(char * array, int sizeArray);
void printOp(int code, int memoryAddress);

// cpu.c
short int readFromRegister(int registerID);
void dumpMemory(unsigned char * memory, long unsigned int memSize);
short int handleIO(int IOIdentifier, bool writing, short int value);
void writeToRegister(int registerID, short int value);
void enterInterruption();
void exitInterruption();
void erasePDS();
void patchMemory(int address, int value, bool byte);
void programRam(char * Line, int addressToWrite);
void writeToRam(short int value, int address);
short int readFromRam(int address);
int getVal(char c);
int getAddressFromLine(char * Line);
int parseHexFile(FILE *fileopened);
void initializePDS16();
void *run();
void *killThread();

// main.c
int main(int argc, char const *argv[]);

// microcode.c
void decodeOp(unsigned int code);
bool carryBorrow(short int number1, short int number2, bool isSum);
bool parity(short int number);
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
int nop(int code);
int iret(int code);

// gui.c
void fixedRegistersPrinting();
bool isOnBreakpointList(int address);
void breakpointManager(int id, int address, bool adding);
void menu();
void initializeGUI();
void parseSymbolsFile(FILE *symbols_file);
unsigned getTermWidth();
unsigned getTermHeight();
void fixedASMPrinting();
void fixedRegistersPrinting();
void printHelp(char * commandHelp);

// gui-windows.c
void serverStart();
void readLoop();

#endif