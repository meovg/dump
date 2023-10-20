#ifndef COMMON_H_
#define COMMON_H_

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#define MEMORY_MAX UINT16_MAX

enum Registers {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,   // program counter
    R_COND, // condition flag
    R_COUNT // number of registers
};

enum ConditionFlag {
    FL_POS = 1 << 0, // P (positive)
    FL_ZRO = 1 << 1, // Z (zero)
    FL_NEG = 1 << 2, // N (negative)
};

enum Opcode {
    OP_BR = 0, // branch
    OP_ADD,    // add
    OP_LD,     // load
    OP_ST,     // store
    OP_JSR,    // jump register
    OP_AND,    // bitwise and
    OP_LDR,    // load register
    OP_STR,    // store register
    OP_RTI,    // unused
    OP_NOT,    // bitwise not
    OP_LDI,    // load indirect
    OP_STI,    // store indirect
    OP_JMP,    // jump
    OP_RES,    // reserved (unused)
    OP_LEA,    // load effective address
    OP_TRAP    // execute trap
};

enum {
    MR_KBSR = 0xfe00, // keyboard status
    MR_KBDR = 0xfe02  // keyboard data
};

enum Trapcode {
    TRAP_GETC = 0x20,  // get character from keyboard, not echoed onto the terminal
    TRAP_OUT = 0x21,   // output a character
    TRAP_PUTS = 0x22,  // output a word string
    TRAP_IN = 0x23,    // get character from keyboard, echoed onto the terminal
    TRAP_PUTSP = 0x24, // output a byte string
    TRAP_HALT = 0x25   // halt the program
};

// ********** defined in utils.c **********

extern uint16_t memory[MEMORY_MAX];  // 65536 locations
extern uint16_t registers[R_COUNT];

uint16_t sign_extend(uint16_t x, int bit_count);
uint16_t swap16(uint16_t x);
void update_flags(uint16_t r);

uint16_t read_object_file(FILE *fh);

void memory_set(uint16_t address, uint16_t val);
uint16_t memory_get(uint16_t address);

// ********** defined in terminal.c **********

void disable_input_buffering(void);
void restore_input_buffering(void);

uint16_t check_key(void);

void handle_interrupt(int signal);

// ********** defined in instructions.h **********

void cmd_add(uint16_t instr);
void cmd_and(uint16_t instr);
void cmd_not(uint16_t instr);
void cmd_br(uint16_t instr);
void cmd_jmp(uint16_t instr);
void cmd_jsr(uint16_t instr);
void cmd_ld(uint16_t instr);
void cmd_ldi(uint16_t instr);
void cmd_ldr(uint16_t instr);
void cmd_lea(uint16_t instr);
void cmd_st(uint16_t instr);
void cmd_sti(uint16_t instr);
void cmd_str(uint16_t instr);
void cmd_trap(uint16_t instr);

#endif