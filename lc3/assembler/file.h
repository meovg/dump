#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint16_t *addrbuf;
    uint16_t *insbuf;
    uint32_t *lnbuf;
    uint16_t ind;
    uint16_t cap;
} FileBuf;

// todo: methods with file buffer

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

const char *extensions[] = {".sym", ".obj", ".lst"};

uint8_t clean(char *filename)
{
    uint8_t err = 0;

    printf("\nCleaning up files produced by the assembler...\n");
    for (uint8_t i = 0; i < 3; i++) {
        replace_extension(filename, extensions[i]);
        printf("Deleting \"%s\"...\n", filename);
        if (remove(filename)) {
            printf("Unable to delete \"%s\"!\n", filename);
            err = 1;
        }
    }
    replace_extension(filename, ".sym");

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