#include "common.h"

uint16_t memory[MEMORY_MAX];  // 65536 locations
uint16_t registers[R_COUNT];

// extends the sign bit (the leftmost bit) to remaining left bits
// when x is cast to 16-bit register value
uint16_t sign_extend(uint16_t x, int bit_count) {
    if ((x >> (bit_count - 1)) & 1)
        x |= (0xffff << bit_count);
    return x;
}

// swaps the bit sequence in higher byte with that of lower byte
uint16_t swap16(uint16_t x) {
    return (x << 8) | (x >> 8);
}

// updates condition flags register
void update_flags(uint16_t r) {
    if (registers[r] == 0)
        registers[R_COND] = FL_ZRO;
    else if (registers[r] >> 15)
        // a 1 in the left-most bit indicates negative sign
        registers[R_COND] = FL_NEG;
    else
        registers[R_COND] = FL_POS;
}

// reads the object file and stores the instructions in memory
uint16_t read_object_file(FILE *file) {
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
    return origin;
}

// writes a 16-bit value to memory at given address
void memory_set(uint16_t address, uint16_t val) {
    memory[address] = val;
}

// gets the content in memory block at given address
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