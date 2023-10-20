#include <string.h>
#include <ctype.h>

#include "common.h"

#define ARR_SIZE(a) (sizeof(a) / sizeof((a)[0]))

int8_t get_register_type(Token *tok) {
    static const char *reg_str[][2] = {
        {"R0", "r0"}, {"R1", "r1"}, {"R2", "r2"}, {"R3", "r3"},
        {"R4", "r4"}, {"R5", "r5"}, {"R6", "r6"}, {"R7", "r7"}
    };

    for (int8_t i = 0; i < ARR_SIZE(reg_str); i++) {
        if (!strcmp(tok->str, reg_str[i][0]) || !strcmp(tok->str, reg_str[i][1]))
            return i;
    }
    return -1;
}

// checks if token is correct branch options or return the default BRnzp
int8_t get_branch_type(Token *tok) {
    static const char *branch_str[][3] = {
        {"BRP", "BRp", "brp"},
        {"BRZ", "BRz", "brz"},
        {"BRZP", "BRzp", "brzp"},
        {"BRN", "BRn", "brn"},
        {"BRNP", "BRnp", "brnp"},
        {"BRNZ", "BRnz", "brnz"},
        {"BRNZP", "BRnzp", "brnzp"},
        {"BR", "BR", "br"}
    };

    for (int8_t i = 0; i < ARR_SIZE(branch_str); i++) {
        for (uint8_t j = 0; j < ARR_SIZE(branch_str[i]); j++) {
            if (!strcmp(tok->str, branch_str[i][j]))
                return i + 1;
        }
    }
    return -1;
}

// checks if token is trapcode or return -1
int8_t get_trap_type(Token *tok) {
    if (!strcmp(tok->str, "TRAP") || !strcmp(tok->str, "trap"))
        return 0;

    static const char *traps_str[][2] = {
        {"GETC", "getc"},
        {"OUT", "out"},
        {"PUTS", "puts"},
        {"IN", "in"},
        {"PUTSP", "putsp"},
        {"HALT", "halt"}
    };

    for (int8_t i = 0; i < ARR_SIZE(traps_str); i++) {
        if (!strcmp(tok->str, traps_str[i][0]) || !strcmp(tok->str, traps_str[i][1]))
            return i + 32;
    }
    return -1;
}

// checks if token is an opcode and return its index or -1
int8_t get_opcode_type(Token *tok) {
    static const char *op_str[][2] = {
        {"BR", "br"},
        {"ADD", "add"},
        {"LD", "ld"},
        {"ST", "st"},
        {"JSR", "jsr"},
        {"AND", "and"},
        {"LDR", "ldr"},
        {"STR", "str"},
        {"RTI", "rti"},
        {"NOT", "not"},
        {"LDI", "ldi"},
        {"STI", "sti"},
        {"JMP", "jmp"},
        {"", ""},
        {"LEA", "lea"},
        {"TRAP", "trap"}
    };

    for (int8_t i = 0; i < ARR_SIZE(op_str); i++) {
        if (!strcmp(tok->str, op_str[i][0]) || !strcmp(tok->str, op_str[i][1]))
            return i == OP_RES ? -1 : i;
    }

    if (get_branch_type(tok) != -1)
        return OP_BR;
    if (get_trap_type(tok) != -1)
        return OP_TRAPS;
    if (!strcmp(tok->str, "JSRR") || !strcmp(tok->str, "jsrr"))
        return OP_JSRR;
    if (!strcmp(tok->str, "RET") || !strcmp(tok->str, "ret"))
        return OP_RET;
    return -1;
}

int8_t get_pseudo_type(Token *tok) {
    static const char *pseudos[][2] = {
        {".ORIG", ".orig"},
        {".END", ".end"},
        {".BLKW", ".blkw"},
        {".FILL", ".fill"},
        {".STRINGZ", ".stringz"}
    };

    for (int8_t i = 0; i < ARR_SIZE(pseudos); i++) {
        if (!strcmp(tok->str, pseudos[i][0]) || !strcmp(tok->str, pseudos[i][1]))
            return i;
    }
    return -1;
}

// checks if token is a valid symbol
// (first character is an alphabetical character or underscore, the rest are
// either alphabetical, underscore or a numeric digit;
uint8_t is_valid_symbol(Token *tok) {
    char *s = tok->str;

    if (!isalpha(s[0]) && s[0] != '_')
        return 0;

    for (uint16_t i = 0; s[i] != '\0'; i++) {
        if (!isalnum(s[i]) && s[i] != '_')
            return 0;
    }
    return 1;
}
