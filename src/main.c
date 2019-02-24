#include "main.h"

// TODO: Clean this file

int main(int argc, char const *argv[])
{
    initializeGUI();

    // Hex File
    FILE *file;
    file = fopen(argv[1], "r");
    if (!file)
    {
        printf(RED "Improper calling!\n\n" RESET);
        printf("Syntax of the programm:\n");
        printf("./PDS16 <hex_file> (optional)<syms_file>\n\n");
        exit(1);
    }
    initializePDS16();
    parseHexFile(file);
    fclose(file);

    // Syms file
    if (argc == 3)
    {
        file = fopen(argv[2], "r");
        if (!file)
        {
            printf(YELLOW "Unable to open symbols file: " RESET "%s\n", argv[2]);
        }
        else
        {
            parseSymbolsFile(file);
            fclose(file);
        }
    }
    else
    {
        char tryingFile[255] = {0};
        memcpy(tryingFile, argv[1], strlen(argv[1]) - 4);
        strcat(tryingFile, ".syms\0");
        printf("No symbols file especified... Trying: %s\n", tryingFile);
        file = fopen(tryingFile, "r");
        if (!file)
        {
            printf(YELLOW "Unable to open symbols file: " RESET "%s\n", tryingFile);
        }
        else
        {
            parseSymbolsFile(file);
            fclose(file);
        }
    }
    // SHA1((unsigned char *)argv[1], strlen(argv[1]), sha1);
    menu();
    return 0;
}

// TODO: Criar suporte para novos ficheiros DONE
// TODO: Criar um novo menu KINDA DONE
// TODO: Disassemble DONE
// TODO: Dump memory to a file DONE
// TODO: Separar e melhor hierarquia dos ficheiros MEH
// TODO: Melhorar a implementação do ISA KINDA DONE
// TODO: Patch Memory DONE
// TODO: Do all the TODOs
// TODO: Clear all the .h files
