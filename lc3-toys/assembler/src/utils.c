#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

char *binseq[16] = {
    "0000", "0001", "0010", "0011",
    "0100", "0101", "0110", "0111",
    "1000", "1001", "1010", "1011",
    "1100", "1101", "1110", "1111"
};

char escape_value(char c) {
    static const char esc_chrs[] = {
        '\'', '\"', '\?', '\\', 'a', 'b', 'e', 'f', 'n', 'r', 't', 'v'
    };
    static const char esc_vals[] = {
        0x27, 0x22, 0x3F, 0x5C, 0x07, 0x08, 0x1B, 0x0C, 0x0A, 0x0D, 0x09, 0x0B
    };

    for (int8_t i = 0; i < 12; i++) {
        if (c == esc_chrs[i])
            return esc_vals[i];
    }
    return c;
}

int8_t is_eow(char c) {
    static const char eow_chrs[] = {
        '\0', '\r', '\t', '\n', ' ', ',', ';'
    };

    int8_t eow = 0;
    for (uint8_t i = 0; i < 7; i++) {
        if (c == eow_chrs[i])
            eow++;
    }

    return eow;
}

int8_t is_eol(char c) {
    static const char eol_chrs[] = {
        '\0', '\n', '\r', ';' // semicolon marks the start of a comment
    };

    int8_t eol = 0;
    for (uint8_t i = 0; i < 4; i++) {
        if (c == eol_chrs[i])
            eol++;
    }

    return eol;
}

void *mymalloc(size_t size) {
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr,"Out of memory while allocating %zu bytes\n", size);
        exit(1);
    }
    return p;
}

void *myrealloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (!p) {
        fprintf(stderr,"Out of memory while allocating %zu bytes\n", size);
        exit(1);
    }
    return p;
}

// error count and warning count when assembler is running
static uint32_t errcnt = 0;
static uint32_t warncnt = 0;

// notify an error on the terminal
void set_error(uint32_t lidx, char *format, ...) {
    va_list args;

    errcnt++;
    va_start(args, format);
    fprintf(stderr, "Error: %d: ", lidx);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

// notify a warning on the terminal
void set_warning(uint32_t lidx, char *format, ...) {
    va_list args;

    warncnt++;
    va_start(args, format);
    fprintf(stderr, "Warning: %d: ", lidx);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

uint32_t get_error_count(void) {
    return errcnt;
}

uint32_t get_warning_count(void) {
    return warncnt;
}