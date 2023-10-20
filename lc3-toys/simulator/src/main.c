#include <signal.h>

#include "common.h"

int main(int argc, const char **argv) {
    if (argc != 2) {
        // show usage string
        printf("%s [obj-file].obj\n", argv[0]);
        exit(2);
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        printf("failed to load image: %s\n", argv[1]);
        exit(1);
    }
    uint16_t pc_start = read_object_file(file);
    fclose(file);

    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    // since exactly one condition flag should be set at any given time
    // set the Z flag
    registers[R_COND] = FL_ZRO;

    // set the PC to starting position
    // 0x3000 is the default
    registers[R_PC] = pc_start;

    for (;;) {
        uint16_t instr = memory_get(registers[R_PC]++);
        uint16_t op = instr >> 12;

        switch (op) {
        case OP_ADD:
            cmd_add(instr); break;
        case OP_AND:
            cmd_and(instr); break;
        case OP_NOT:
            cmd_not(instr); break;
        case OP_BR:
            cmd_br(instr); break;
        case OP_JMP:
            cmd_jmp(instr); break;
        case OP_JSR:
            cmd_jsr(instr); break;
        case OP_LD:
            cmd_ld(instr); break;
        case OP_LDI:
            cmd_ldi(instr); break;
        case OP_LDR:
            cmd_ldr(instr); break;
        case OP_LEA:
            cmd_lea(instr); break;
        case OP_ST:
            cmd_st(instr); break;
        case OP_STI:
            cmd_sti(instr); break;
        case OP_STR:
            cmd_str(instr); break;
        case OP_TRAP:
            cmd_trap(instr); break;
        case OP_RTI:
            printf("RTI handler is not available\n");
            exit(1);
        case OP_RES:
            break;
        default:
            exit(1);
        }
    }
    return 0;
}