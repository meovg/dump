#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lexer.h"

typedef struct {
    uint16_t *addrs;
    uint16_t *instrs;
    uint32_t *lns;
    uint16_t size;
    uint16_t cap;
} FileBuf;

// todo: methods with file buffer

#define FILEBUF_DEFAULT_SIZE 128

FileBuf *filebuf_new(void)
{
    FileBuf *buf = malloc(sizeof *buf);
    buf->size = 0;
    buf->cap = FILEBUF_DEFAULT_SIZE;
    buf->addrs = malloc(buf->cap * sizeof(uint16_t));
    buf->instrs = malloc(buf->cap * sizeof(uint16_t));
    buf->lns = malloc(buf->cap * sizeof(uint32_t));
    return buf;
}

void filebuf_add(FileBuf *buf, uint16_t addr, uint16_t instr, uint32_t ln)
{
    if (buf->size == buf->cap) {
        buf->cap *= 2;
        buf->addrs = realloc(buf->addrs, buf->cap * sizeof(uint16_t));
        buf->instrs = realloc(buf->instrs, buf->cap * sizeof(uint16_t));
        buf->lns = realloc(buf->lns, buf->cap * sizeof(uint32_t));
    }
    buf->addrs[buf->size] = addr;
    buf->instrs[buf->size] = instr;
    buf->lns[buf->size] = ln;
    buf->size++;
}

void filebuf_free(FileBuf *buf)
{
    if (!buf) return;
    if (buf->addrs) free(buf->addrs);
    if (buf->instrs) free(buf->instrs);
    if (buf->lns) free(buf->lns);
    free(buf);
}

typedef struct {
    FILE *src;
    FILE *sym;
    FILE *obj;
    FILE *lst;
} FileList;

int8_t check_extension(char *filename, const char *ext)
{
    char *s = strrchr(filename, '.');
    if (!s) return 0;
    return !strcmp(s, ext);
}

void replace_extension(char *filename, const char *ext)
{
    char *s = strrchr(filename, '.');
    for (int i = 0; s[i] != '\0'; i++) {
        s[i] = ext[i];
    }
}

uint8_t open_asm(FileList *fl, char *filename)
{
    uint8_t err = 0;

    fl->src = fopen(filename, "r");
    if (fl->src == NULL) {
        printf("Unable to open assembly file: %s!\n", filename);
        err++;
    }

    replace_extension(filename, ".sym");
    fl->sym = fopen(filename, "w");
    if (fl->sym == NULL) {
        printf("Unable to create symbol table file: %s!\n", filename);
        err++;
    }

    replace_extension(filename, ".obj");
    fl->obj = fopen(filename, "w");
    if (fl->obj == NULL) {
        printf("Unable to create object file: %s!\n", filename);
        err++;
    }

    replace_extension(filename, ".lst");
    fl->lst = fopen(filename, "w");
    if (fl->lst == NULL) {
        printf("Unable to create list file: %s!\n", filename);
        err++;
    }

    replace_extension(filename, ".asm");

    return err;
}

uint8_t clean(char *filename)
{
    uint8_t err = 0;
    static const char *exts[] = {".sym", ".obj", ".lst"};

    printf("\nCleaning up files produced by the assembler...\n");
    for (uint8_t i = 0; i < 3; i++) {
        replace_extension(filename, exts[i]);
        printf("Deleting \"%s\"...\n", filename);
        if (remove(filename)) {
            printf("Unable to delete \"%s\"!\n", filename);
            err = 1;
        }
    }
    replace_extension(filename, ".asm");

    return err;
}

void close_files(FileList *fl)
{
    if (!fl) return;
    if (fl->src) fclose(fl->src);
    if (fl->sym) fclose(fl->sym);
    if (fl->obj) fclose(fl->obj);
    if (fl->lst) fclose(fl->lst);
}

// todo: output file buffer to .lst, .sym and .obj files

void output_obj(FileBuf *buf, FILE *obj)
{
    uint16_t byte_cnt = buf->size;
    uint8_t tmp[byte_cnt];
    for (uint16_t i = 0; i < buf->size; i++) {
        tmp[i*2] = buf->instrs[i] >> 8; // high byte
        tmp[i*2+1] = buf->instrs[i] & 0xff; // low byte
    }
    fwrite(tmp, sizeof(uint8_t), byte_cnt, obj);
}

void output_lst(FileBuf *buf, FILE *lst, FILE *src)
{
    static const char *hextobin[16] = {
        "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
        "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111",
    };

    char line[MAX_LEN + 1];
    uint16_t idx = 1;
    uint16_t lim = buf->size;

    fseek(src, 0, SEEK_SET); // find beginning of .asm file
    fprintf(lst, "  Addr  |  Hex  |       Bin        | Line |  Source\n");

    for (uint32_t ln = 1; fgets(line, MAX_LEN + 1, src); ln++) {
        if (idx != lim && buf->lns[idx] == ln) {
            uint16_t instr = buf->instrs[idx];
            uint16_t addr = buf->addrs[idx];

            fprintf(lst, " x%04X  | x%04X | %s%s%s%s ",
                addr,
                instr,
                hextobin[instr >> 12],
                hextobin[(instr >> 8) & 0xf],
                hextobin[(instr >> 4) & 0xf],
                hextobin[instr & 0xf]
            );
            idx++;
        } else {
            fprintf(lst, "        |       |                  ");
        }

        fprintf(lst, "| %4d | %s", ln, line);

        // multiple instructions case
        while (idx != lim && buf->lns[idx] == ln) {
            uint16_t instr = buf->instrs[idx];
            fprintf(lst, "        | x%04X | %s%s%s%s |      |\n",
                instr,
                hextobin[instr >> 12],
                hextobin[(instr >> 8) & 0xf],
                hextobin[(instr >> 4) & 0xf],
                hextobin[instr & 0xf]
            );
            idx++;
        }
    }
}