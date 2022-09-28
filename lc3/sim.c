#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <signal.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#   include <windows.h>
#   include <conio.h>
#elif defined(__linux__)
#   include <termios.h>
#   include <unistd.h>
#   include <fcntl.h>
#   include <sys/time.h>
#   include <sys/types.h>
#   include <sys/mman.h>
#endif

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

enum {
    TRAP_GETC = 0x20,  // get character from keyboard, not echoed onto the terminal
    TRAP_OUT = 0x21,   // output a character
    TRAP_PUTS = 0x22,  // output a word string
    TRAP_IN = 0x23,    // get character from keyboard, echoed onto the terminal
    TRAP_PUTSP = 0x24, // output a byte string
    TRAP_HALT = 0x25   // halt the program
};

#define MEMORY_MAX UINT16_MAX

uint16_t memory[MEMORY_MAX];  // 65536 locations
uint16_t registers[R_COUNT];

#define PC_START 0x3000

#if defined(_WIN32) || defined(__CYGWIN__)
HANDLE input_handle;
DWORD fdw_mode, fdw_original_mode;
#elif defined(__linux__)
struct termios original_tio;
#endif

void restore_input_buffering(void);

// save the original input mode and
// disable line-by-line input and showing input in terminal
void disable_input_buffering(void) {
    atexit(restore_input_buffering);

#if defined(_WIN32) || defined(__CYGWIN__)
    input_handle = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(input_handle, &fdw_original_mode); // save original mode

    fdw_mode = fdw_original_mode
            ^ ENABLE_ECHO_INPUT  // no input echo
            ^ ENABLE_LINE_INPUT; // return when one or more characters are available

    SetConsoleMode(input_handle, fdw_mode); // set new mode
    FlushConsoleInputBuffer(input_handle); // clear buffer

#elif defined(__linux__)
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;

    new_tio.c_lflag &= ~(ICANON  // turn off canonical mode and read byte by byte
                       | ECHO);  // no input echo
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

#endif
}

// restore the original input mode, used after the program exits
void restore_input_buffering(void) {
#if defined(_WIN32) || defined(__CYGWIN__)
    SetConsoleMode(input_handle, fdw_original_mode);

#elif defined(__linux__)
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);

#endif
}

// check if a key is pressed
uint16_t check_key(void) {
#if defined(_WIN32) || defined(__CYGWIN__)
    return WaitForSingleObject(input_handle, 1000) == WAIT_OBJECT_0 && _kbhit();

#elif defined(__linux__)
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    return select(1, &readfds, NULL, NULL, &timeout) != 0;

#endif
}

void handle_interrupt(int signal) {
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

// extend the sign bit (the leftmost bit) to remaining left bits
// when x is cast to 16-bit register value
uint16_t sign_extend(uint16_t x, int bit_count) {
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xffff << bit_count);
    }
    return x;
}

// swap the bit sequence in higher byte with that of lower byte
uint16_t swap16(uint16_t x) {
    return (x << 8) | (x >> 8);
}

// update condition flags register
void update_flags(uint16_t r) {
    if (registers[r] == 0) {
        registers[R_COND] = FL_ZRO;
    } else if (registers[r] >> 15) {
        // a 1 in the left-most bit indicates negative sign
        registers[R_COND] = FL_NEG;
    } else {
        registers[R_COND] = FL_POS;
    }
}

// read the object file and store the instructions in memory
void read_object_file(FILE *file) {
    // the origin tells us where in memory to place the image
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    // we know the maximum file size so we only need one fread
    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t *p = memory + origin;
    size_t r = fread(p, sizeof(uint16_t), max_read, file);

    // swap to little endian
    while (r-- > 0) {
        *p = swap16(*p);
        ++p;
    }
}

// load an object file into memory
int import_object_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return 0;
    };

    read_object_file(file);
    fclose(file);
    return 1;
}

// write a 16-bit value to memory at given address
void memory_set(uint16_t address, uint16_t val) {
    memory[address] = val;
}

// get the content in memory block at given address
// if the address is MR_KBSR, also check keyboard input and store it in MR_KBDR
uint16_t memory_get(uint16_t address) {
    if (address == MR_KBSR) {
        if (check_key()) {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        } else {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}

// add two values and store the result in register
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

// perform bitwise and two values and store the result in register
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

// store binary negation of a value and store it in register
// this is a unary operator, so there's only 1 source register needed
void cmd_not(uint16_t instr) {
    uint16_t dr = (instr >> 9) & 0x7;
    uint16_t sr = (instr >> 6) & 0x7;

    registers[dr] = ~registers[sr];
    update_flags(dr);
}

// make machine jump to another instruction by an offset if condition is met
void cmd_br(uint16_t instr) {
    uint16_t pc_offset = sign_extend(instr & 0x1ff, 9);
    uint16_t cond_flag = (instr >> 9) & 0x7;

    // check if any flag in cond_flag is set in cond register
    // if yes, pc counter points to instruction guided by pc_offset
    // otherwise, pc counter points to the next instruction
    if (cond_flag & registers[R_COND]) {
        registers[R_PC] += pc_offset;
    }
}

// make machine move to an instruction specified by the register w/o conditions
void cmd_jmp(uint16_t instr) {
    // also handle RET when sr1 (base register position) is 7
    uint16_t sr1 = (instr >> 6) & 0x7;
    registers[R_PC] = registers[sr1];
}

// perform jump register to start subroutine
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

// load a value into a register
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

    // add pc_offset to the current PC, look at that memory location to get the final address
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

// store the content of register sr into memory
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

// interact with I/O devices (in this case, a terminal)
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

        case TRAP_PUTS: {
            // one char per word
            uint16_t *c = memory + registers[R_R0];
            while (*c) {
                putc((char)*c, stdout);
                ++c;
            }
            fflush(stdout);
        } break;

        case TRAP_IN: {
            // print out a prompt for inputting a char, echo the input char
            // and the char is stored in register R0
            printf("Enter a character: ");
            char c = getchar();
            putc(c, stdout);
            fflush(stdout);
            registers[R_R0] = (uint16_t)c;
            update_flags(R_R0);
        } break;

        case TRAP_PUTSP: {
            // one char per byte (two bytes per word)
            // here we need to swap back to big endian format
            uint16_t *c = memory + registers[R_R0];
            while (*c) {
                char char1 = (*c) & 0xff;
                putc(char1, stdout);
                char char2 = (*c) >> 8;
                if (char2) putc(char2, stdout);
                ++c;
            }
            fflush(stdout);
        } break;

        case TRAP_HALT:
            puts("HALT");
            fflush(stdout);

        default: exit(1);
    }
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        // show usage string
        printf("%s [obj-file1] ...\n", argv[0]);
        exit(2);
    }

    for (int j = 1; j < argc; ++j) {
        if (!import_object_file(argv[j])) {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    // since exactly one condition flag should be set at any given time, set the Z flag
    registers[R_COND] = FL_ZRO;

    // set the PC to starting position
    // 0x3000 is the default
    registers[R_PC] = PC_START;

    while (1) {
        uint16_t instr = memory_get(registers[R_PC]++);
        uint16_t op = instr >> 12;

        switch (op) {
            case OP_ADD: cmd_add(instr); break;
            case OP_AND: cmd_and(instr); break;
            case OP_NOT: cmd_not(instr); break;
            case OP_BR: cmd_br(instr); break;
            case OP_JMP: cmd_jmp(instr); break;
            case OP_JSR: cmd_jsr(instr); break;
            case OP_LD: cmd_ld(instr); break;
            case OP_LDI: cmd_ldi(instr); break;
            case OP_LDR: cmd_ldr(instr); break;
            case OP_LEA: cmd_lea(instr); break;
            case OP_ST: cmd_st(instr); break;
            case OP_STI: cmd_sti(instr); break;
            case OP_STR: cmd_str(instr); break;
            case OP_TRAP: cmd_trap(instr); break;
            case OP_RES:
            case OP_RTI:
            default: exit(1);
        }
    }
    return 0;
}
