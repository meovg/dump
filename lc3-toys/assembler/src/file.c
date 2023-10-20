#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"

int8_t extension_check(char *path, const char *ext) {
    char *s = strrchr(path, '.');
    if (!s)
        return 0;
    return !strcmp(s, ext);
}

void extension_replace(char *path, const char *ext) {
    char *s = strrchr(path, '.');
    for (int i = 0; s[i] != '\0'; i++)
        s[i] = ext[i];
}

// this function is simply initialization so allocate FileList object yourself,
void filelist_open(FileList *fl, char *path) {
    fl->src = fopen(path, "r");
    if (fl->src == NULL)
        set_error(0, "Couldn't open assembly file: %s", path);

    extension_replace(path, ".sym");
    fl->sym = fopen(path, "w");
    if (fl->sym == NULL)
        set_error(0, "Couldn't create symbol table file: %s", path);

    extension_replace(path, ".obj");
    fl->obj = fopen(path, "wb");
    if (fl->obj == NULL)
        set_error(0, "Couldn't create object file: %s", path);

    extension_replace(path, ".bin");
    fl->bin = fopen(path, "w");
    if (fl->bin == NULL)
        set_error(0, "Couldn't create binary file: %s", path);

    extension_replace(path, ".lst");
    fl->lst = fopen(path, "w");
    if (fl->lst == NULL)
        set_error(0, "Couldn't create listing file: %s", path);

    extension_replace(path, ".asm");
}

void filelist_close(FileList *fl) {
    if (!fl)
        return;
    if (fl->src)
        fclose(fl->src);
    if (fl->sym)
        fclose(fl->sym);
    if (fl->obj)
        fclose(fl->obj);
    if (fl->bin)
        fclose(fl->bin);
    if (fl->lst)
        fclose(fl->lst);
}

// deletes unused output files, used when set_errors occur
void clean_output(char *path) {
    static const char *exts[] = {".sym", ".obj", ".bin", ".lst"};
    uint8_t i;

    printf("\nCleaning up output files...\n");
    for (i = 0; i < 4; i++) {
        extension_replace(path, exts[i]);
        if (remove(path))
            set_warning(0, "Couldn't delete %s", path);
    }
    extension_replace(path, ".asm");
}

// sequence used to print instructions in binary
#define PRbinseq(x) \
    binseq[x >> 12], \
    binseq[(x >> 8) & 0xf], \
    binseq[(x >> 4) & 0xf], \
    binseq[x & 0xf]

// writes assembled machine instructions to .obj file
void write_obj(OutputBuf *obuf, FILE *obj) {
    // (the struggle to swap the byte order of machine instructions
    // on both the assembler and simulator just because LC-3 uses big endian)
    // i guess object file is written last then...
    for (uint16_t i = 0; i < obuf->size; i++) {
        uint16_t tmp = obuf->instr[i];
        obuf->instr[i] = (tmp << 8) | (tmp >> 8);
    }
    fwrite(obuf->instr, sizeof(uint16_t), obuf->size, obj);
}

// writes machine code in .bin text file
void write_bin(OutputBuf *obuf, FILE *bin) {
    for (uint16_t i = 0; i < obuf->size; i++)
        fprintf(bin, "%s%s%s%s\n", PRbinseq(obuf->instr[i]));
}

// writes the symbol table to .sym file
// symbols are listed in the second column while their addr are in the first
void write_sym(SymTab *st, FILE *sym) {
    fprintf(sym, "  Addr  |  Symbol\n");

    for (uint32_t i = 0; i < st->bktcnt; i++) {
        Symbol *bucket_iter = st->buckets[i];

        while (bucket_iter != NULL) {
            fprintf(sym, " x%04X  |  %s\n", bucket_iter->addr, bucket_iter->key->str);
            bucket_iter = bucket_iter->next;
        }
    }
}

// writes listing table to .lst file
// each column of table specifies assembled machine code addr in hex (1)
// instructions in hex (2) and binary (3), the index (4) and content (5)
// of respective source codes in assembly file
//
// note that a line in source code can map to 0, 1 or many machine instructions
void write_lst(OutputBuf *obuf, FILE *lst, FILE *src) {
    static char line[MAX_LINE_LEN + 1];
    uint16_t i = 1;
    uint32_t lidx;

    fseek(src, 0, SEEK_SET);
    fprintf(lst, "  Addr  |  Hex  |       Bin        | Line |  Source\n");

    for (lidx = 1; fgets(line, MAX_LINE_LEN + 1, src); lidx++) {
        // write first 3 columns if any matched instruction is found
        // or leave them blank
        if (i != obuf->size && obuf->lidx[i] == lidx) {
            uint16_t instr = obuf->instr[i];
            uint16_t addr = obuf->addr[i];
            fprintf(lst, " x%04X  | x%04X | %s%s%s%s ", addr, instr, PRbinseq(instr));
            i++;
        } else {
            fprintf(lst, "        |       |                  ");
        }

        // write respective line in source code
        // fgets reads newline as well so there's no need to add \n
        fprintf(lst, "| %4d | %s", lidx, line);

        // write other machine codes in case an assembly instruction maps
        // to multiple machine codes (.STRINGZ and .BLKW)
        while (i != obuf->size && obuf->lidx[i] == lidx) {
            uint16_t instr = obuf->instr[i];
            fprintf(lst, "        | x%04X | %s%s%s%s |      |\n", instr, PRbinseq(instr));
            i++;
        }
    }
}
