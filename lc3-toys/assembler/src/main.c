#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv) {
    // show usage guide
    if (argc != 2) {
        printf("%s [asm file]\n", argv[0]);
        exit(1);
    }

    // check if filename argument is an assembly file;
    if (!extension_check(argv[1], ".asm")) {
        fprintf(stderr, "%s is not an assembly file\n", argv[1]);
        exit(2);
    }

    OutputBuf *obuf = outputbuf_new();
    InputBuf *ibuf = inputbuf_new();
    SymTab *st = symtab_new();
    FileList *fl = mymalloc(sizeof *fl);

    assemble(ibuf, obuf, st, fl, argv[1]);

    filelist_close(fl);
    free(fl);
    inputbuf_del(ibuf);
    outputbuf_del(obuf);
    symtab_del(st);

    if (get_error_count() > 0)
        clean_output(argv[1]);

    printf("Yields %d error(s) and %d warning(s)\n",
           get_error_count(), get_warning_count());
    return 0;
}
