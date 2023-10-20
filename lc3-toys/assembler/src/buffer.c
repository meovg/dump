#include <stdlib.h>

#include "common.h"

InputBuf *inputbuf_new(void) {
    InputBuf *ibuf = mymalloc(sizeof *ibuf);

    ibuf->cap = INPUTBUF_DEFAULT_SIZE;
    ibuf->size = 0;
    ibuf->arr = mymalloc(ibuf->cap * sizeof(Line *));
    return ibuf;
}

void inputbuf_add(InputBuf *ibuf, Line *ln) {
    if (ibuf->size == ibuf->cap) {
        ibuf->cap *= 2;
        ibuf->arr = myrealloc(ibuf->arr, ibuf->cap * sizeof(Line *));
    }

    ibuf->arr[ibuf->size] = ln;
    ibuf->size++;
}

// retrieves a Line object based on the index
Line *inputbuf_get(InputBuf *ibuf, uint32_t idx) {
    if (idx >= ibuf->size)
        return NULL;
    return ibuf->arr[idx];
}

// deallocates Line list and the lines contained
void inputbuf_del(InputBuf *ibuf) {
    if (!ibuf)
        return;
    for (uint32_t i = 0; i < ibuf->size; i++)
        line_del(ibuf->arr[i]);
    free(ibuf->arr);
    free(ibuf);
}

OutputBuf *outputbuf_new(void) {
    OutputBuf *obuf = mymalloc(sizeof *obuf);

    obuf->size = 0;
    obuf->cap = OUTPUTBUF_DEFAULT_SIZE;
    obuf->addr = mymalloc(obuf->cap * sizeof(uint16_t));
    obuf->instr = mymalloc(obuf->cap * sizeof(uint16_t));
    obuf->lidx = mymalloc(obuf->cap * sizeof(uint32_t));
    return obuf;
}

// adds assembled instructions, address and linenum to output buffer
// to be used later on
void outputbuf_add(OutputBuf *obuf, uint16_t addr, uint16_t instr, uint32_t lidx) {
    if (obuf->size == obuf->cap) {
        obuf->cap *= 2;
        obuf->addr = myrealloc(obuf->addr, obuf->cap * sizeof(uint16_t));
        obuf->instr = myrealloc(obuf->instr, obuf->cap * sizeof(uint16_t));
        obuf->lidx = myrealloc(obuf->lidx, obuf->cap * sizeof(uint32_t));
    }

    obuf->addr[obuf->size] = addr;
    obuf->instr[obuf->size] = instr;
    obuf->lidx[obuf->size] = lidx;
    obuf->size++;
}

void outputbuf_del(OutputBuf *obuf) {
    if (!obuf)
        return;
    if (obuf->addr)
        free(obuf->addr);
    if (obuf->instr)
        free(obuf->instr);
    if (obuf->lidx)
        free(obuf->lidx);
    free(obuf);
}
