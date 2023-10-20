#include "common.h"

// adds two values and stores the result in register
void cmd_add(uint16_t instr) {
    // destination register
    uint16_t dr = (instr >> 9) & 0x7;

    // first operand register position
    uint16_t sr1 = (instr >> 6) & 0x7;

    // whether we are in immediate mode or register mode
    uint16_t imm_flag = (instr >> 5) & 0x1;

    // in immediate mode, the second operand is the rightmost 5 bits
    // in register mode, the second operand is in the register sr2
    if (imm_flag) {
        uint16_t imm5 = sign_extend(instr & 0x1f, 5);
        registers[dr] = registers[sr1] + imm5;
    } else {
        // second operand register position
        uint16_t sr2 = instr & 0x7;
        registers[dr] = registers[sr1] + registers[sr2];
    }

    // update cond flags based on dr as it contains the result
    update_flags(dr);
}

// performs bitwise and two values and stores the result in register
void cmd_and(uint16_t instr) {
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t sr1 = (instr >> 6) & 0x7;
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if (imm_flag) {
        uint16_t imm5 = sign_extend(instr & 0x1f, 5);
        registers[dr] = registers[sr1] & imm5;
    } else {
        uint16_t sr2 = instr & 0x7;
        registers[dr] = registers[sr1] & registers[sr2];
    }
    update_flags(dr);
}

// stores binary negation of a value and stores it in register
// this is a unary operator, so there's only 1 source register needed
void cmd_not(uint16_t instr) {
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t sr = (instr >> 6) & 0x7;

    registers[dr] = ~registers[sr];
    update_flags(dr);
}

// makes machine jump to another instruction by an offset if condition is met
void cmd_br(uint16_t instr) {
    uint16_t pc_offset = sign_extend(instr & 0x1ff, 9);
    uint16_t cond_flag = (instr >> 9) & 0x7;

    // check if any flag in cond_flag is set in cond register
    // if yes, pc counter points to instruction guided by pc_offset
    // otherwise, pc counter points to the next instruction
    if (cond_flag & registers[R_COND])
        registers[R_PC] += pc_offset;
}

// makes machine move to an instruction specified by the register
// w/o conditions
void cmd_jmp(uint16_t instr) {
    // also handle RET when sr1 (base register position) is 7
    uint16_t sr1 = (instr >> 6) & 0x7;
    registers[R_PC] = registers[sr1];
}

// performs jump register to start subroutine
void cmd_jsr(uint16_t instr) {
    // bit 11 indicates addressing mode
    uint16_t long_flag = (instr >> 11) & 1;

    // save the current instruction PC points to (return address) in R7
    registers[R_R7] = registers[R_PC];

    if (long_flag) {
        // JSR case: similar to unconditional BR
        uint16_t long_pc_offset = sign_extend(instr & 0x7ff, 11);
        registers[R_PC] += long_pc_offset;
    } else {
        // JSRR case: similar to JMP
        uint16_t sr1 = (instr >> 6) & 0x7;
        registers[R_PC] = registers[sr1];
    }
}

// loads a value into a register
// the value is in memory with address being the sum of PC counter and offset
void cmd_ld(uint16_t instr) {
    // destination register (DR)
    uint16_t dr = (instr >> 9) & 0x7;

    // PC offset 9
    uint16_t pc_offset = sign_extend(instr & 0x1ff, 9);

    registers[dr] = memory_get(registers[R_PC] + pc_offset);
    update_flags(dr);
}

// similar to LD but instead of loading the value in memory
// the machine loads the content in memory with value being the address
void cmd_ldi(uint16_t instr) {
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1ff, 9);

    // add pc_offset to the current PC, look at that memory location to get
    // the final address
    registers[dr] = memory_get(memory_get(registers[R_PC] + pc_offset));
    update_flags(dr);
}

// similar to LD but the base instruction address is not from the PC counter
void cmd_ldr(uint16_t instr) {
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t sr1 = (instr >> 6) & 0x7;
    uint16_t offset = sign_extend(instr & 0x3f, 6);
    registers[dr] = memory_get(registers[sr1] + offset);
    update_flags(dr);
}

// load effective address
// similar to LD but it loads the address instead
void cmd_lea(uint16_t instr) {
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1ff, 9);
    registers[dr] = registers[R_PC] + pc_offset;
    update_flags(dr);
}

// stores the content of register sr into memory
// with address being the sum of PC counter and PCoffset9
void cmd_st(uint16_t instr) {
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1ff, 9);
    memory_set(registers[R_PC] + pc_offset, registers[dr]);
}

// similar to ST but the destination address is the content in
// memory with sum of PC counter and PCoffset9 as address
void cmd_sti(uint16_t instr) {
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1ff, 9);
    memory_set(memory_get(registers[R_PC] + pc_offset), registers[dr]);
}

// similar to ST but the address is the sum of value in register sr1
void cmd_str(uint16_t instr) {
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t sr1 = (instr >> 6) & 0x7;
    uint16_t offset = sign_extend(instr & 0x3f, 6);
    memory_set(registers[sr1] + offset, registers[dr]);
}

// handles trap routines
void cmd_trap(uint16_t instr) {
    registers[R_R7] = registers[R_PC];

    switch (instr & 0xff) {
    case TRAP_GETC:
        // read a single ASCII char
        registers[R_R0] = (uint16_t)getchar();
        update_flags(R_R0);
        break;

    case TRAP_OUT:
        // write out a single char
        putc((char)registers[R_R0], stdout);
        fflush(stdout);
        break;

    case TRAP_PUTS:
        {
            // one char per word
            uint16_t *c = memory + registers[R_R0];
            while (*c) {
                putc((char)*c, stdout);
                ++c;
            }
            fflush(stdout);
        }
        break;

    case TRAP_IN:
        {
            // print out a prompt for inputting a char, echo the input char
            // and the char is stored in register R0
            printf("Enter a character: ");
            char c = getchar();
            putc(c, stdout);
            fflush(stdout);
            registers[R_R0] = (uint16_t)c;
            update_flags(R_R0);
        }
        break;

    case TRAP_PUTSP:
        {
            // one char per byte (two bytes per word)
            // here we need to swap back to big endian format
            uint16_t *c = memory + registers[R_R0];
            while (*c) {
                char char1 = (*c) & 0xff;
                putc(char1, stdout);
                char char2 = (*c) >> 8;
                if (char2)
                    putc(char2, stdout);
                ++c;
            }
            fflush(stdout);
        }
        break;

    case TRAP_HALT:
        puts("-ACK!");
        fflush(stdout);

    default:
        exit(1);
    }
}
