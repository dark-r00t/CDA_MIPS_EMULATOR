#include "spimcore.h"

/*
Coded by:
    Jan Darge
    Jason Rock
*/

/* ALU */
/* 10 Points */
void ALU (unsigned A, unsigned B, char ALUControl, unsigned *ALUresult, char *Zero) {
    // PROCESS ALU OPCODE
    switch (ALUControl) {
        case 0: // Addition
            *ALUresult = A + B;
            break;
        case 1: // Subtraction
            *ALUresult = A - B;
            break;
        case 2: // SLT (Int)
            *ALUresult = ((int) A < (int) B) ? 1 : 0;
            break;
        case 3: // SLT (Unisgned)
            *ALUresult = (A < B) ? 1 : 0;
            break;
        case 4: // BITWISE AND 'A' and 'B'
            *ALUresult = (A & B);
            break;
        case 5: // BITWISE OR 'A' and 'B'
            *ALUresult = (A | B);
            break;
        case 6: // SHIFT B LEFTY BY 16 BITS
            *ALUresult = B << 16;
            break;
        case 7: // BITWISE NOT ON 'A'
            *ALUresult = ~A;
            break;
    }

    // SET "Zero" BASED ON THE "ALUresult"
    *Zero = (*ALUresult == 0) ? 1 : 0;
}

/* instruction fetch */
/* 10 Points */
int instruction_fetch (unsigned PC, unsigned *Mem, unsigned *instruction) {
    if ((PC > 0xFFFC) || (PC % 4 != 0)) {
        return 1;
    }

    *instruction = MEM(PC);

    return 0;
}

/* instruction partition */
/* 10 Points */
void instruction_partition (unsigned instruction, unsigned *op, unsigned *r1, unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec) {
    *op = (instruction & 0xFC000000) >> 26;
    *r1 = (instruction & 0x3E00000) >> 21;
    *r2 = (instruction & 0x1F0000) >> 16;
    *r3 = (instruction & 0xF800) >> 11;
    *funct = (instruction & 0x3F);
    *offset = (instruction & 0xFFFF);
    *jsec = (instruction & 0x3FFFFFF);
}

/* instruction decode */
/* 15 Points */
int instruction_decode (unsigned op, struct_controls *controls) {

    controls->Jump = 0;
    controls->Branch = 0;
    controls->MemRead = 0;
    controls->MemtoReg = 0;
    controls->MemWrite = 0;
    controls->RegDst = 0;
    controls->RegWrite = 0;
    controls->ALUSrc = 0;
    controls->ALUOp = 0;

    switch (op) {
        case (0) : // R-type
            controls->RegDst = 1;
            controls->RegWrite = 1;
            controls->ALUOp = 7;
            break;

        case (2) : // j - Jump
            controls->Jump = 1;
            break;

        case (4) : // beq - Branch on equal
            controls->Branch = 1;
            controls->ALUOp = 1;
            break;

        case (8) : // addi - Add immediate
            controls->RegWrite = 1;
            controls->ALUSrc = 1;
            break;

        case (10) : // slti - Set less than immediate
            controls->RegWrite = 1;
            controls->ALUSrc = 1;
            controls->ALUOp = 2;
            break;

        case (11) : // sltiu - Set less than immediate unsigned
            controls->RegWrite = 1;
            controls->ALUSrc = 1;
            controls->ALUOp = 3;
            break;

        case (15) : // lui - Load upper immediate
            controls->RegWrite = 1;
            controls->ALUSrc = 1;
            controls->ALUOp = 6;
            break;

        case (35) : // lw - Load word
            controls->MemRead = 1;
            controls->MemtoReg = 1;
            controls->RegWrite = 1;
            controls->ALUSrc = 1;
            break;

        case (43) : // sw - Store word
            controls->MemWrite = 1;
            controls->ALUSrc = 1;
            break;

        default : // Invalid
            return 1;
    }

    return 0;
}

/* Read Register */
/* 5 Points */
void read_register (unsigned r1, unsigned r2, unsigned *Reg, unsigned *data1, unsigned *data2) {
    *data1 = Reg[r1];
    *data2 = Reg[r2];
}

/* Sign Extend */
/* 10 Points */
void sign_extend (unsigned offset, unsigned *extended_value) {
    unsigned negative = offset >> 15;

    *extended_value = (negative) ? (offset | 0xFFFF0000) : (offset & 0x0000FFFF);
}

/* ALU operations */
/* 10 Points */
int ALU_operations (unsigned data1, unsigned data2, unsigned extended_value, unsigned funct, char ALUOp, char ALUSrc, unsigned *ALUresult, char *Zero) {

    char ALUControl = 0;

    if(ALUOp == 7) {
        switch(funct) {
            case (0) : //shift left
                ALUControl = 6;
                break;
            case (32) : //addition
                ALUControl = 0;
                break;
            case (34) : //subtraction
                ALUControl = 1;
                break;
            case (36) : //and
                ALUControl = 4;
                break;
            case (37) : //or
                ALUControl = 5;
                break;
            case (39) : //not (nor)
                ALUControl = 7;
                break;
            case (42) : //set less than
                ALUControl = 2;
                break;
            case (43) : //set less than unsigned
                ALUControl = 3;
                break;
            default:
                return 1;
        }

        ALU(data1, data2, ALUControl, ALUresult, Zero);
        return 0;
    }

    else if ((ALUOp >= 0) && (ALUOp < 7)) {
        //I-Type instruction (or J-type, but not really)
        ALU(data1, extended_value, ALUOp, ALUresult, Zero);
        return 0;

    }

    return 1;
}

/* Read / Write Memory */
/* 10 Points */
int rw_memory (unsigned ALUresult, unsigned data2, char MemWrite, char MemRead, unsigned *memdata, unsigned *Mem) {
    if ((!MemRead && !MemWrite) || (MemRead && MemWrite)) {
        // No memory operations occurring. Have a nice day!
        return 0;
    }

    if (!(ALUresult % 4) && (ALUresult >> 2) > 0xFFFC) {
        return 1;
    }

    if (MemRead) {
        *memdata = MEM(ALUresult);
    }

    if (MemWrite) {
        MEM(ALUresult) = data2;
    }

    return 0;
}

/* Write Register */
/* 10 Points */
void write_register (unsigned r2, unsigned r3, unsigned memdata, unsigned ALUresult, char RegWrite, char RegDst, char MemtoReg, unsigned *Reg) {
    if (RegWrite) {
        unsigned temp = (RegDst) ? r3 : r2;
        Reg[temp] = (MemtoReg) ? memdata : ALUresult;
    }
}

/* PC update */
/* 10 Points */
void PC_update (unsigned jsec, unsigned extended_value, char Branch, char Jump, char Zero, unsigned *PC) {

    *PC += 4;

    if (Jump) {
        *PC = (jsec << 2) & (*PC | 0xF0000000);
    } else if (Branch && Zero) {
        *PC += (extended_value << 2);
    }
}
