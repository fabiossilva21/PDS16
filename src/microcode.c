#include "microcode.h"

bool parity(int number){
        int parity = number & 1;
        for(int i = 1; i <= 15; i++){
                parity += ((1 << i)&number)>>i;
        }
        parity = parity & 0x1;
        return parity;
}

void ldi(int code){
        int rd = code & 0b111;
        int i = (code>>3) & 0b11111111;
        pds16.registers[rd] = i;
}

void ldih(int code){
        int rd = code & 0b111;
        int i = (code & (0b11111111<<3))<<5;
        pds16.registers[rd] = pds16.registers[rd]+i;
}

void ld(int code){
        int opcode = (code>>11)&0b11111;
        bool w = (code>>10)&1;
        int rd = code & 0b111;
        if (opcode == 0b10){
                int dir7 = (code>>3)&0b1111111;
                if (w){
                        pds16.registers[rd] = (pds16.mem[dir7] << 8) + pds16.mem[dir7+1];
                }else{
                        pds16.registers[rd] = pds16.mem[dir7];
                }
        }else if (opcode == 0b11){
                int idri = (code>>6)&0b111;
                int rb = (code>>3)&0b111;
                if (((code>>9)&0b1) == 0){
                        if (w){
                                pds16.registers[rd] = (pds16.mem[pds16.registers[rb]+(idri)*2] << 8)
                                + pds16.mem[pds16.registers[rb]+(idri)*2+1];
                        }else{
                                pds16.registers[rd] = pds16.mem[pds16.registers[rb]+idri];
                        }
                }else{
                        if (w){
                                pds16.registers[rd] = (pds16.mem[pds16.registers[rb]+pds16.registers[idri]] << 8)
                                + pds16.mem[pds16.registers[rb]+pds16.registers[idri]+1];
                        }else{
                                pds16.registers[rd] = pds16.mem[pds16.registers[rb]+pds16.registers[idri]];
                        }
                }
        }
}

void st(int code){
        int opcode = (code>>11)&0b11111;
        bool w = (code>>10)&1;
        int rs = code & 0b111;
        if (opcode == 0b110){
                int dir7 = (code>>3)&0b1111111;
                if (w){
                        pds16.mem[dir7] = ((pds16.registers[rs]&0xFF00)>>8);
                        pds16.mem[dir7+1] = pds16.registers[rs]&0xFF;
                }else{
                        pds16.mem[dir7] = pds16.registers[rs]&0xFF;
                }
        }else if (opcode == 0b111){
                int idri = (code>>6)&0b111;
                int rb = (code>>3)&0b111;
                if (((code>>9)&0b1) == 0){
                        if (w){
                                pds16.mem[pds16.registers[rb]+idri*2] = (pds16.registers[rs]&0xff00)>>8;
                                pds16.mem[pds16.registers[rb]+idri*2+1] = (pds16.registers[rs]&0xff);
                        }else{
                                pds16.mem[pds16.registers[rb]+idri] = pds16.registers[rs]&0xff;
                        }
                }else{
                        if (w){
                                pds16.mem[pds16.registers[rb]+pds16.registers[idri]] = (pds16.registers[rs]&0xff00)>>8;
                                pds16.mem[pds16.registers[rb]+pds16.registers[idri]+1] = (pds16.registers[rs]&0xff);
                        }else{
                                pds16.mem[pds16.registers[rb]+pds16.registers[idri]] = pds16.registers[rs]&0xff;
                        }
                }
        }
}

void add(int code){
        int opcode = (code >> 11) & 0b11111;
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        bool f = (code>>10) & 0b1;
        if (opcode == 0b10000){
                int rn = (code>>6) & 0b111;
                bool r = (code>>9) & 0b1;
                if (!r){
                        pds16.registers[rd] = pds16.registers[rm] + pds16.registers[rn];
                }
        }else{
                int constant = (code>>6)&0b1111;
                pds16.registers[rd] = pds16.registers[rm] + constant;
        }
        if(!f){
                return;
        }
        printf("TODO: Flags\n");
}

void adc(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int rn = (code>>6) & 0b111;
        bool r = (code>>9) & 0b1;
        bool f = (code>>10) & 0b1;
        bool c = (pds16.registers[6] & 0b10)>>1;
        pds16.registers[rd] = pds16.registers[rm] + pds16.registers[rn] + c;
        if(!f){
                return;
        }
        printf("TODO: Flags\n");
}

void sub(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int rn = (code>>6) & 0b111;
        bool r = (code>>9) & 0b1;
        bool f = (code>>10) & 0b1;
        pds16.registers[rd] = pds16.registers[rm] - pds16.registers[rn];
        if(!f){
                return;
        }
        printf("TODO: Flags\n");
}

void sbb(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int rn = (code>>6) & 0b111;
        bool r = (code>>9) & 0b1;
        bool f = (code>>10) & 0b1;
        bool c = (pds16.registers[6] & 0b10)>>1;
        pds16.registers[rd] = pds16.registers[rm] - pds16.registers[rn] - c;
        if(!f){
                return;
        }
        printf("TODO: Flags\n");
}

void anl(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int rn = (code>>6) & 0b111;
        bool r = (code>>9) & 0b1;
        bool f = (code>>10) & 0b1;
        pds16.registers[rd] = pds16.registers[rm] & pds16.registers[rn];
        if(!f){
                return;
        }
        pds16.registers[6] &= 0b111101;
        if((pds16.registers[rm]&pds16.registers[rn]) == 0) {
                pds16.registers[6] |= 1;
        }
        pds16.registers[6] |= (parity((pds16.registers[rm]&pds16.registers[rn]))<<3);
        printf("TODO: Flags\n");
}

void orl(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int rn = (code>>6) & 0b111;
        bool r = (code>>9) & 0b1;
        bool f = (code>>10) & 0b1;
        pds16.registers[rd] = pds16.registers[rm] | pds16.registers[rn];
        if(!f){
                return;
        }
        pds16.registers[6] &= 0b111101;
        if((pds16.registers[rm]|pds16.registers[rn]) == 0) {
                pds16.registers[6] |= 1;
        }
        pds16.registers[6] |= (parity((pds16.registers[rm]|pds16.registers[rn]))<<3);
        printf("TODO: Flags\n");
}

void xrl(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int rn = (code>>6) & 0b111;
        bool r = (code>>9) & 0b1;
        bool f = (code>>10) & 0b1;
        pds16.registers[rd] = pds16.registers[rm] ^ pds16.registers[rn];
        if(!f){
                return;
        }
        pds16.registers[6] &= 0b111101;
        if((pds16.registers[rm]^pds16.registers[rn]) == 0) {
                pds16.registers[6] |= 1;
        }
        pds16.registers[6] |= (parity((pds16.registers[rm]^pds16.registers[rn]))<<3);
        printf("TODO: Flags\n");
}

void not(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        bool r = (code>>9) & 0b1;
        bool f = (code>>10) & 0b1;
        pds16.registers[rd] = ~pds16.registers[rm];
        if(!f){
                return;
        }
        pds16.registers[6] &= 0b111101;
        if(~(pds16.registers[rm]) == 0) {
                pds16.registers[6] |= 1;
        }
        pds16.registers[6] |= (parity(~pds16.registers[rm])<<3);
}

void shl(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int c4 = (code>>6) & 0b1111;
        int in = (code>>10) & 0b1;
        // Flags = 0 (GE = 0)
        pds16.registers[6] &= 0x10;
        // Carry
        pds16.registers[6] |= ((pds16.registers[rm]&(0b1<<(16-c4)))>>14);
        // Operation
        pds16.registers[rd] = (pds16.registers[rm]<<c4);
        // Zero
        pds16.registers[6] |= (pds16.registers[rd] == 0) ? 1 : 0;
        // Parity
        pds16.registers[6] |= (parity(pds16.registers[rd])<<3);
        if(!in) return;
        for(int i = 1; i <= c4; i++){
                pds16.registers[rd] = pds16.registers[rd] + (in<<(c4-i));
        }
}

void shr(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int c4 = (code>>6) & 0b1111;
        int in = (code>>10) & 0b1;
        // Flags = 0 (GE = 0)
        pds16.registers[6] &= 0x10;
        // Carry
        pds16.registers[6] |= ((pds16.registers[rm]>>(c4-1))&1)<<1;
        // Operation
        pds16.registers[rd] = (pds16.registers[rm]>>c4);
        // Zero
        pds16.registers[6] |= (pds16.registers[rd] == 0) ? 1 : 0;
        // Parity
        pds16.registers[6] |= (parity(pds16.registers[rd])<<3);
        if(!in) return;
        for(int i = 1; i <= c4; i++){
                pds16.registers[rd] = pds16.registers[rd] + (0x8000>>(c4-i));
        }
}

void rr(int code){
        // Flags = 0 (GE = 0)
        pds16.registers[6] &= 0x10;
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int c4 = (code>>6) & 0b1111;
        pds16.registers[rd] = pds16.registers[rm];
        // RRM?
        if(((code>>10)&0b111111) == 0b111111){
                int a15 = (pds16.registers[rd]&0x8000);
                //printf("%x", pds16.registers[rm]);
                for(int i = 1; i <= c4; i++){
                        pds16.registers[rd] = (((pds16.registers[rd])>>1) | a15);
                }
        }else { // RRL
                for(int i = 1; i <= c4; i++){
                        int cy = pds16.registers[rd]&0x1;
                        pds16.registers[rd] = (((pds16.registers[rd])>>1) | (cy<<15));
                }
        }
        // Carry
        pds16.registers[6] |= ((pds16.registers[rm]>>(c4-1))&1)<<1;
        // Parity
        pds16.registers[6] |= (parity(pds16.registers[rd])<<3);
        // Zero
        pds16.registers[6] |= (pds16.registers[rd] == 0) ? 1 : 0;
}


void jz(int code){
        if ((pds16.registers[6] & 0x1) != 0){
                jmp(code);
        }
}

void jnz(int code){
        if ((pds16.registers[6] & 0x1) == 0){
                jmp(code);
        }
}

void jc(int code){
        if ((pds16.registers[6] & 0x2) != 0){
                jmp(code);
        }
}

void jnc(int code){
        if ((pds16.registers[6] & 0x2) == 0){
                jmp(code);
        }
}

void jmp(int code){
        int rb = code & 0b111;
        int8_t off = (code>>3) & 0b11111111;
        pds16.registers[7] = pds16.registers[rb] + off*2;
}

void jmpl(int code){
        pds16.registers[5] = pds16.registers[7];
        jmp(code);
}

void iret(){
        if((pds16.registers[6] & 0x20) == 0){
                sendError("Tried to return from an interrupt routine while not in one!\n");
        }else{
                exitInterruption();
        }
}
