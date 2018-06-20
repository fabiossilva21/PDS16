#include "main.h"

int main(int argc, char const *argv[]) {
        initializePDS16();
        FILE *file;
        file = fopen(argv[1], "r");
        parseHexFile(pds16.mem, file);
        fclose(file);
        loop();
        return 0;
}

// TODO: Criar suporte para novos ficheiros
// TODO: Criar um novo menu
// TODO: Disasseble
// TODO: Dump memory to a file
// TODO: Separar e melhor hierarquia dos ficheiros
// TODO: Melhorar a implementação do ISA 
