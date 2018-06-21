#include "microcode.h"

bool carryBorrow(short int number1, short int number2, bool isSum){
        if (isSum){
                short int sum = number1+number2;
                if(((number1&number2)&0x8000) != 0){
                        return 1;
                }
                if((sum&0x8000) == 0 && (number1)&0x8000 != 0){
                        return 1;
                }
                if((sum&0x8000) == 0 && (number2)&0x8000 != 0){
                        return 1;
                }
        }else{
                number2 ^= 0xFFFF;
                short int sub = number1+number2;
                if(((number1&number2)&0x8000) != 0){
                        return 1^!isSum;
                }
                if((sub&0x8000) == 0 && (number1)&0x8000 != 0){
                        return 1^!isSum;
                }
                if((sub&0x8000) == 0 && (number2)&0x8000 != 0){
                        return 1^!isSum;
                }
                return 0^!isSum;
        }
        return 0;
}

bool parity(short int number){
        short int parity = number & 1;
        for(int i = 1; i <= 15; i++){
                parity += ((1 << i)&number)>>i;
        }
        parity = parity & 0x1;
        return parity;
}

void ldi(int code){
        int rd = code & 0b111;
        int i = (code>>3) & 0b11111111;
        writeToRegister(rd, i);
}

void ldih(int code){
        int rd = code & 0b111;
        int i = (code & (0b11111111<<3))<<5;
        writeToRegister(rd, readFromRegister(rd)+i);
}

void ld(int code){
        int opcode = (code>>11)&0b11111;
        bool w = (code>>10)&1;
        int rd = code & 0b111;
        if (opcode == 0b10){
                int dir7 = (code>>3)&0b1111111;
                if (w){
                        writeToRegister(rd, (pds16.mem[dir7] << 8) + pds16.mem[dir7+1]);
                }else{
                        writeToRegister(rd, pds16.mem[dir7]);
                }
        }else if (opcode == 0b11){
                int idri = (code>>6)&0b111;
                int rb = (code>>3)&0b111;
                if (((code>>9)&0b1) == 0){
                        if (w){
                                writeToRegister(rd, (pds16.mem[readFromRegister(rb)+idri*2]<<8) +
                                pds16.mem[readFromRegister(rb)+(idri)*2+1]);
                        }else{
                                writeToRegister(rd, pds16.mem[readFromRegister(rb)+idri]);
                        }
                }else{
                        if (w){
                                writeToRegister(rd, (pds16.mem[readFromRegister(rb)+readFromRegister(idri)] << 8)
                                + pds16.mem[readFromRegister(rb)+readFromRegister(idri)+1]);
                        }else{
                                writeToRegister(rd, pds16.mem[readFromRegister(rb)+readFromRegister(idri)]);
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
                        pds16.mem[dir7] = ((readFromRegister(rs)&0xFF00)>>8);
                        pds16.mem[dir7+1] = readFromRegister(rs)&0xFF;
                }else{
                        pds16.mem[dir7] = readFromRegister(rs)&0xFF;
                }
        }else if (opcode == 0b111){
                int idri = (code>>6)&0b111;
                int rb = (code>>3)&0b111;
                if (((code>>9)&0b1) == 0){
                        if (w){
                                pds16.mem[readFromRegister(rb)+idri*2] = (readFromRegister(rs)&0xff00)>>8;
                                pds16.mem[readFromRegister(rb)+idri*2+1] = (readFromRegister(rs)&0xff);
                        }else{
                                pds16.mem[readFromRegister(rb)+idri] = readFromRegister(rs)&0xff;
                        }
                }else{
                        if (w){
                                pds16.mem[readFromRegister(rb)+readFromRegister(idri)] = (readFromRegister(rs)&0xff00)>>8;
                                pds16.mem[readFromRegister(rb)+readFromRegister(idri)+1] = (readFromRegister(rs)&0xff);
                        }else{
                                pds16.mem[readFromRegister(rb)+readFromRegister(idri)] = readFromRegister(rs)&0xff;
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
                if (f){
                        // Flags = 0
                        pds16.registers[6] &= 0x30;
                        // Z
                        pds16.registers[6] |= (((readFromRegister(rm)+readFromRegister(rn)) == 0) ? 1 : 0);
                        // Carry
                        pds16.registers[6] |= (carryBorrow(readFromRegister(rm), readFromRegister(rn), 1)<<1);
                        // GE
                        pds16.registers[6] |= (readFromRegister(rm)>readFromRegister(rn)? 0b100 : 0);
                        // Parity
                        pds16.registers[6] |= parity(readFromRegister(rm) + readFromRegister(rn))<<3;
                }
                if (r){
                        writeToRegister(rd, readFromRegister(rm)+readFromRegister(rn));
                }
        }else{
                int constant = (code>>6)&0b1111;
                if(f){
                        // Flags = 0
                        pds16.registers[6] &= 0x30;
                        // Z
                        pds16.registers[6] |= (((readFromRegister(rm)+constant)&0xFFFF) == 0) ? 1 : 0;
                        // Carry
                        pds16.registers[6] |= carryBorrow(readFromRegister(rm), constant, 1)<<1;
                        // GE
                        pds16.registers[6] |= (readFromRegister(rm)>constant? 0b100 : 0);
                        // Parity Odd
                        pds16.registers[6] |= parity(readFromRegister(rm)+constant)<<3;
                }
                writeToRegister(rd, readFromRegister(rm)+constant);
        }
}

void adc(int code){
        int opcode = (code >> 11) & 0b11111;
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        bool f = (code>>10) & 0b1;
        bool c = (readFromRegister(6) & 0b10)>>1;
        if (opcode == 0x10010){
                bool r = (code>>9) & 0b1;
                int rn = (code>>6) & 0b111;
                if (f){
                        // Flags = 0
                        pds16.registers[6] &= 0x30;
                        // Z
                        pds16.registers[6] |= (((readFromRegister(rm)+(readFromRegister(rn)+c)) == 0) ? 1 : 0);
                        // Carry
                        pds16.registers[6] |= carryBorrow(readFromRegister(rm), (readFromRegister(rn)+c), 1)<<1;
                        // GE
                        pds16.registers[6] |= (readFromRegister(rm)>(readFromRegister(rn)+c)? 0b100 : 0);
                        // Parity
                        pds16.registers[6] |= parity(readFromRegister(rm) + (readFromRegister(rn)+c))<<3;
                }
                if(r){
                        writeToRegister(rd, readFromRegister(rm)+readFromRegister(rn)+c);
                }
        }else{
                int constant = (code>>6)&0b1111;
                if(f){
                        // Flags = 0
                        pds16.registers[6] &= 0x30;
                        // Z
                        pds16.registers[6] |= (((readFromRegister(rm)+constant+c)&0xFFFF) == 0) ? 1 : 0;
                        // Carry
                        pds16.registers[6] |= carryBorrow(readFromRegister(rm), constant+c, 1)<<1;
                        // GE
                        pds16.registers[6] |= (readFromRegister(rm)>(constant+c)? 0b100 : 0);
                        // Parity Odd
                        pds16.registers[6] |= parity(readFromRegister(rm)+constant+c)<<3;
                }
                writeToRegister(rd, readFromRegister(rm)+constant+c);
        }
}

void sub(int code){
        int opcode = (code >> 11) & 0b11111;
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        bool f = (code>>10) & 0b1;
        if (opcode == 0b10001){
                bool r = (code>>9) & 0b1;
                int rn = (code>>6) & 0b111;
                if (f){
                        // Flags = 0
                        pds16.registers[6] &= 0x30;
                        // Z
                        pds16.registers[6] |= (((readFromRegister(rm)-(readFromRegister(rn))) == 0) ? 1 : 0);
                        // Carry
                        pds16.registers[6] |= (carryBorrow(readFromRegister(rm), ~(readFromRegister(rn)+1), 1)<<1);
                        // GE
                        pds16.registers[6] |= (readFromRegister(rm)>readFromRegister(rn)? 0b100 : 0);
                        // Parity
                        pds16.registers[6] |= parity(readFromRegister(rm) - (readFromRegister(rn)))<<3;
                }
                if(r){
                        writeToRegister(rd, readFromRegister(rm)-readFromRegister(rn));
                }
        }else{
                int constant = (code>>6)&0b1111;
                if (f){
                        // Flags = 0
                        pds16.registers[6] &= 0x30;
                        // Z
                        pds16.registers[6] |= (((readFromRegister(rm)-constant)) == 0) ? 1 : 0;
                        // Carry
                        pds16.registers[6] |= carryBorrow(readFromRegister(rm), constant, 0)<<1;
                        // GE
                        pds16.registers[6] |= (readFromRegister(rm)>(constant)? 0b100 : 0);
                        // Parity
                        pds16.registers[6] |= parity(readFromRegister(rm) - constant)<<3;
                }
                writeToRegister(rd, (readFromRegister(rm) - constant));
        }
}

void sbb(int code){
        int opcode = (code >> 11) & 0b11111;
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        bool f = (code>>10) & 0b1;
        int c = (pds16.registers[6] & 0b10)>>1;
        if (opcode == 0b10011){
                bool r = (code>>9) & 0b1;
                int rn = (code>>6) & 0b111;
                if (f){
                        // Flags = 0
                        pds16.registers[6] &= 0x30;
                        // Z
                        pds16.registers[6] |= (((readFromRegister(rm)-(readFromRegister(rn)-c)) == 0) ? 1 : 0);
                        // Carry
                        pds16.registers[6] |= carryBorrow(readFromRegister(rm), (readFromRegister(rn)-c), 0)<<1;
                        // GE
                        pds16.registers[6] |= (readFromRegister(rm)>(readFromRegister(rn)-c)? 0b100 : 0);
                        // Parity
                        pds16.registers[6] |= parity(readFromRegister(rm) - (readFromRegister(rn)-c))<<3;
                }
                if(r){
                        writeToRegister(rd, readFromRegister(rm)-readFromRegister(rn)-c);
                }
        }else{
                int constant = (code>>6)&0b1111;
                if (f){
                        // Flags = 0
                        pds16.registers[6] &= 0x30;
                        // Z
                        pds16.registers[6] |= (((readFromRegister(rm)-constant-c) == 0) ? 1 : 0);
                        // Carry
                        pds16.registers[6] |= carryBorrow(readFromRegister(rm), constant-c, 0)<<1;
                        // GE
                        pds16.registers[6] |= ((readFromRegister(rm)>(constant-c))? 0b100 : 0);
                        // Parity
                        pds16.registers[6] |= parity(readFromRegister(rm) - constant - c)<<3;
                }
                writeToRegister(rd, (readFromRegister(rm) - constant - c));
        }
}

void anl(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int rn = (code>>6) & 0b111;
        bool r = (code>>9) & 0b1;
        bool f = (code>>10) & 0b1;
        writeToRegister(rd, readFromRegister(rm)&readFromRegister(rn));
        if(!f){
                return;
        }
        pds16.registers[6] &= 0b111101;
        if((readFromRegister(rm)&readFromRegister(rn)) == 0) {
                pds16.registers[6] |= 1;
        }
        pds16.registers[6] |= (parity(readFromRegister(rm)&readFromRegister(rn))<<3);
        printf("TODO: Flags\n");
}

void orl(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int rn = (code>>6) & 0b111;
        bool r = (code>>9) & 0b1;
        bool f = (code>>10) & 0b1;
        writeToRegister(rd, readFromRegister(rm)|readFromRegister(rn));
        if(!f){
                return;
        }
        pds16.registers[6] &= 0b111101;
        if((readFromRegister(rm)|readFromRegister(rn)) == 0) {
                pds16.registers[6] |= 1;
        }
        pds16.registers[6] |= (parity((readFromRegister(rm)|readFromRegister(rn))<<3));
        printf("TODO: Flags\n");
}

void xrl(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int rn = (code>>6) & 0b111;
        bool r = (code>>9) & 0b1;
        bool f = (code>>10) & 0b1;
        writeToRegister(rd, readFromRegister(rm)^readFromRegister(rn));
        if(!f){
                return;
        }
        pds16.registers[6] &= 0b111101;
        if(readFromRegister(rm)^readFromRegister(rn) == 0) {
                pds16.registers[6] |= 1;
        }
        pds16.registers[6] |= parity((readFromRegister(rm)^readFromRegister(rn))<<3);
        printf("TODO: Flags\n");
}

void not(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        bool r = (code>>9) & 0b1;
        bool f = (code>>10) & 0b1;
        writeToRegister(rd, ~readFromRegister(rm));
        if(!f){
                return;
        }
        pds16.registers[6] &= 0b111101;
        if(~(readFromRegister(rm)) == 0) {
                pds16.registers[6] |= 1;
        }
        pds16.registers[6] |= (parity(~readFromRegister(rm))<<3);
}

void shl(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int c4 = (code>>6) & 0b1111;
        int in = (code>>10) & 0b1;
        // Flags = 0 (GE = 0)
        pds16.registers[6] &= 0x30;
        // Carry
        pds16.registers[6] |= ((readFromRegister(rm)&(0b1<<(16-c4)))>>14);
        // Operation
        writeToRegister(rd, readFromRegister(rm)<<c4);
        // Zero
        pds16.registers[6] |= (readFromRegister(rd) == 0) ? 1 : 0;
        // Parity
        pds16.registers[6] |= (parity(readFromRegister(rd))<<3);
        if(!in) return;
        for(int i = 1; i <= c4; i++){
                writeToRegister(rd, readFromRegister(rd)+(in<<(c4-i)));
        }
}

void shr(int code){
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int c4 = (code>>6) & 0b1111;
        int in = (code>>10) & 0b1;
        // Flags = 0 (GE = 0)
        pds16.registers[6] &= 0x30;
        // Carry
        pds16.registers[6] |= ((readFromRegister(rm)>>(c4-1))&1)<<1;
        // Operation
        writeToRegister(rd, readFromRegister(rm)>>c4);
        // Zero
        pds16.registers[6] |= (readFromRegister(rd) == 0) ? 1 : 0;
        // Parity
        pds16.registers[6] |= (parity(readFromRegister(rd))<<3);
        if(!in) return;
        for(int i = 1; i <= c4; i++){
                writeToRegister(rd, readFromRegister(rd)+(0x8000>>(c4-i)));
        }
}

void rr(int code){
        // Flags = 0 (GE = 0)
        pds16.registers[6] &= 0x30;
        int rd = code & 0b111;
        int rm = (code>>3) & 0b111;
        int c4 = (code>>6) & 0b1111;
        writeToRegister(rd, readFromRegister(rm));
        // RRM?
        if(((code>>10)&0b111111) == 0b111111){
                int a15 = readFromRegister(rd)&0x8000;
                for(int i = 1; i <= c4; i++){
                        writeToRegister(rd, (readFromRegister(rd)>>1)|a15);
                }
        }else { // RRL
                for(int i = 1; i <= c4; i++){
                        int cy = readFromRegister(rd)&0x1;
                        writeToRegister(rd, (readFromRegister(rd)>>1)|(cy<<15));
                }
        }
        // Carry
        pds16.registers[6] |= ((readFromRegister(rm)>>(c4-1))&1)<<1;
        // Parity
        pds16.registers[6] |= (parity(readFromRegister(rd))<<3);
        // Zero
        pds16.registers[6] |= (readFromRegister(rd) == 0) ? 1 : 0;
}


void jz(int code){
        if ((readFromRegister(6) & 0x1) != 0){
                jmp(code);
        }
}

void jnz(int code){
        if ((readFromRegister(6) & 0x1) == 0){
                jmp(code);
        }
}

void jc(int code){
        if ((readFromRegister(6) & 0x2) != 0){
                jmp(code);
        }
}

void jnc(int code){
        if ((readFromRegister(6) & 0x2) == 0){
                jmp(code);
        }
}

void jmp(int code){
        int rb = code & 0b111;
        int8_t off = (code>>3) & 0b11111111;
        writeToRegister(7, readFromRegister(rb)+off*2);
}

void jmpl(int code){
        writeToRegister(5, readFromRegister(7));
        jmp(code);
}

void iret(){
        if((readFromRegister(6) & 0x20) == 0){
                sendError("Tried to return from an interrupt routine while not in one!\n");
        }else{
                sendError("Interrupt routine still not implemented =(");
                // exitInterruption();
        }
}
