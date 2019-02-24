#include "microcode.h"

void writeToRam(short int value, int address)
{
    address = address & 0xFFFF;
    if (address >= 0 && address <= 0x7FFF)
    {
        pds16.mem[address] = value;
    }
    else if (address >= 0xFF00 && address <= 0xFF3F)
    {
        pds16.nCS_Out = value;
    }
    else if (address >= 0xFF40 && address <= 0xFF7F)
    {
        pds16.nCS_EXT0_Out = value;
    }
    else if (address >= 0xFF80 && address <= 0xFFBF)
    {
        pds16.nCS_EXT1_Out = value;
    }
    else
    {
        printf(RED "Failed: " RESET "Setting memory address 0x%04x! Invalid Address\n", address);
    }
}

short int readFromRam(int address)
{
    address = address & 0xFFFF;
    if (address >= 0 && address <= 0x7FFF)
    {
        return pds16.mem[address];
    }
    else if (address >= 0xFF00 && address <= 0xFF3F)
    {
        return pds16.nCS_In;
    }
    else if (address >= 0xFF40 && address <= 0xFF7F)
    {
        return pds16.nCS_EXT0_In;
    }
    else if (address >= 0xFF80 && address <= 0xFFBF)
    {
        return pds16.nCS_EXT1_In;
    }
    else
    {
        printf(RED "Failed: " RESET "Read from memory address 0x%04x! Invalid Address.\n", address);
        printf("Program ended.\n");
        exit(-1);
        return 0;
    }
}

short int readFromRegister(int registerID)
{
    if (registerID < 0 || registerID > 7)
    {
        sendError("We just tried to read outside the register bank!");
        printf("PC: %d\n", readFromRegister(7));
        printf("Register ID: %d\n", registerID);
    }
    // Are we in an interrupt routine?
    if ((pds16.registers[6] & 0x20) != 0 && registerID < 6)
    {
        return pds16.iregisters[registerID];
    }
    else
    {
        return pds16.registers[registerID];
    }
}

void writeToRegister(int registerID, short int value)
{
    if (registerID < 0 || registerID > 7)
    {
        printf("Register ID: %d\n", registerID);
        printf("PC: %d\n", readFromRegister(7));
        sendError("We just tried to write outside the register bank!");
    }
    // Are we in an interrupt routine?
    if ((pds16.registers[6] & 0x20) != 0 && registerID < 6)
    {
        pds16.iregisters[registerID] = value & 0xFFFF;
    }
    else
    {
        pds16.registers[registerID] = value & 0xFFFF;
    }
}

short int handleIO(int IOIdentifier, bool writing, short int value)
{
    // 1 -> nCS_Out
    // 2 -> nCS_In
    // 3 -> nCS0_Out
    // 4 -> nCS0_In
    // 5 -> nCS1_Out
    // 6 -> nCS1_In
    switch (IOIdentifier)
    {
    case (1):
        if (writing)
        {
            pds16.nCS_Out = value;
            return 1;
        }
        else
        {
            return pds16.nCS_Out;
        }
    case (2):
        if (writing)
        {
            pds16.nCS_In = value;
            return 1;
        }
        else
        {
            return pds16.nCS_In;
        }
    case (3):
        if (writing)
        {
            pds16.nCS_EXT0_Out = value;
            return 1;
        }
        else
        {
            return pds16.nCS_EXT0_Out;
        }
    case (4):
        if (writing)
        {
            pds16.nCS_EXT0_In = value;
            return 1;
        }
        else
        {
            return pds16.nCS_EXT0_In;
        }
    case (5):
        if (writing)
        {
            pds16.nCS_EXT1_Out = value;
            return 1;
        }
        else
        {
            return pds16.nCS_EXT1_Out;
        }
        break;
    case (6):
        if (writing)
        {
            pds16.nCS_EXT1_In = value;
            return 1;
        }
        else
        {
            return pds16.nCS_EXT1_In;
        }
    default:
        sendWarning("Tried to set an un-existing IO Port");
        return -1;
    }
}

void enterInterruption()
{
    if ((readFromRegister(6) & 0x10) != 0)
    {
        // Copy PSW to iR0
        pds16.iregisters[0] = pds16.registers[6];
        // Set BS and set IE to false
        writeToRegister(6, 0x20);
        // Copy PC to LINK
        writeToRegister(5, readFromRegister(7));
        // Put a 0x2 in PC
        writeToRegister(7, 2);
        sendWarning("Entered the interrupt routine!");
    }
    else if ((readFromRegister(6) & 0x20) != 0)
    {
        sendWarning("We cannot enter an interrupt routine while being in one!");
    }
    else
    {
        sendWarning("Just tried to enter the Interruption routine without IE bit set!");
    }
}

void exitInterruption()
{
    if ((readFromRegister(6) & 0x20) == 0)
    {
        sendError("We got out of scope and tried to exit an interrupt routine without being in one!");
    }
    // Move LINK to PC
    writeToRegister(7, readFromRegister(5));
    // Move iR0 to PSW
    writeToRegister(6, readFromRegister(0));
    sendWarning("Exited the interrupt routine!");
}

void patchMemory(int address, int value, bool byte)
{
    if (address >= 0xFF00 && address <= 0xFF39)
    {
        value = value & 0xffff;
        handleIO(1, true, value);
        return;
    }
    else if (address >= 0xFF40 && address <= 0xFF79)
    {
        value = value & 0xffff;
        handleIO(3, true, value);
        return;
    }
    else if (address >= 0xFF80 && address <= 0xFFBF)
    {
        value = value & 0xffff;
        handleIO(5, true, value);
        return;
    }
    if ((address > 0x7fff || address < 0x0) && (address > 0xFF3F || address < 0xFF00))
    {
        printf("Cannot write 0x%04x to 0x%04x\n", value, address);
        return;
    }
    if (byte)
    {
        writeToRam((value & 0xff), address);
        printf("Sucessfully patched memory address 0x%02x with 0x%02x!\n", address, value & 0xff);
    }
    else
    {
        address &= 0xfffe; // Let's remove the last byte
        writeToRam((value & 0xff00) >> 8, address);
        writeToRam((value & 0xff), address + 1);
        printf("Sucessfully patched memory address 0x%02x and 0x%02x with 0x%02x and 0x%02x!\n", address, address + 1, (value & 0xff00) >> 8, value & 0xff);
    }

    return;
}

void programRam(char *Line, int addressToWrite)
{
    int limit = (addressToWrite & 0xff0000) >> 16;
    addressToWrite = addressToWrite & 0xffff;
    for (int i = 8; i < limit * 2 + 8; i += 2)
    {
        writeToRam(Line[i] * 16 + Line[i + 1], addressToWrite);
        addressToWrite++;
    }
}

// TODO: Next 3 functions on their respective .c
int getVal(char c)
{
    int rtVal = 0;

    if (c >= '0' && c <= '9')
    {
        rtVal = c - '0';
    }
    else if (c >= 'A' && c <= 'F')
    {
        rtVal = c - 'A' + 10;
    }
    else
    {
        rtVal = c - 'a' + 10;
    }
    return rtVal;
}

int getAddressFromLine(char *Line)
{
    int number = 0;
    number = (Line[5]) + (Line[4]) * 16 + (Line[3]) * pow(16, 2) + (Line[2]) * pow(16, 3) +
             (Line[1]) * pow(16, 4) + Line[0] * pow(16, 5);
    return number;
}

int parseHexFile(FILE *fileopened)
{
    int c, i = 0;
    char line[2555];
    while ((c = fgetc(fileopened)) != EOF)
    {
        if (c == ':')
        {
            c = fgetc(fileopened);
        }
        if (c != '\n')
        {
            line[i] = getVal(c);
            i++;
        }
        else
        {
            if (line[0] == 0 && line[1] == 0)
                return 1;
            int memLoc = getAddressFromLine(line);
            printf("0x%04x ", memLoc);
            i = 0;
            programRam(line, memLoc);
        }
    }
    return 1;
}

void initializePDS16()
{
    // Force everything to be 0
    memset(pds16.mem, 0x00, MEMSIZE * sizeof(unsigned char));
    memset(pds16.registers, 0x00, NUM_REGISTERS * sizeof(short int));
    memset(pds16.iregisters, 0x00, NUM_IREGISTERS * sizeof(short int));
    memset(&pds16.nCS_In, 0x00, sizeof(short int));
    memset(&pds16.nCS_Out, 0x00, sizeof(short int));
    memset(&pds16.nCS_EXT0_In, 0x00, sizeof(short int));
    memset(&pds16.nCS_EXT1_In, 0x00, sizeof(short int));
    memset(&pds16.nCS_EXT0_Out, 0x00, sizeof(short int));
    memset(&pds16.nCS_EXT1_Out, 0x00, sizeof(short int));
}
