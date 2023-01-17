#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_TOKEN_NUM 7
#define MAX_TOKEN_LEN 1025
#define MAX_LINE_LEN 7175

typedef struct token {
    uint16_t size;
    char *str;
} Token;

typedef struct tokenlist {
    uint8_t size;
    Token **arr;
} TokenList;

char get_escape_value(char c)
{
    static const char escape_chars[] = {'\'', '\"', '\?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'};
    static const char escape_vals[] = {0x27, 0x22, 0x3F, 0x5C, 0x07, 0x08, 0x0C, 0x0A, 0x0D, 0x09, 0x0B};

    for (uint8_t i = 0; i < 11; i++) {
        if (c == escape_chars[i]) {
            return escape_vals[i];
        }
    }

    return c;
}

void copy_token(Token *dst, const Token *src)
{
    dst->size = src->size;

    if (dst->str) {
        dst->str = realloc(dst->str, dst->size * sizeof(char));
    } else {
        dst->str = malloc(dst->size * sizeof(char));
    }
    memcpy(dst->str, src->str, dst->size * sizeof(char));
}

Token *token_from_string(const char *line, uint16_t *index_ptr)
{
    uint16_t i = *index_ptr;
    uint16_t j = 0;
    char tmp[MAX_TOKEN_LEN];

    while (line[i] != '\0' && line[i] != '\"') {
        if (line[i] == '\\') {
            tmp[j] = get_escape_value(line[++i]);
            j++;
            i++;
        } else {
            tmp[j] = line[i];
            j++;
            i++;
        }
    }

    *index_ptr = i;

    Token *t = malloc(sizeof *t);

    t->size = j + 1;
    t->str = malloc(t->size * sizeof(char));

    for (uint16_t k = 0; k < j; k++) {
        t->str[k] = tmp[k];
    }
    t->str[j] = '\0';

    return t;
}

int8_t is_eow(char c)
{
    int8_t eow = 0;
    static const char eowchars[] = {0, 13, '\t', '\n', ' ', ',', ';'};

    for (uint8_t i = 0; i < 7; i++) {
        if (c == eowchars[i]) {
            eow++;
        }
    }
    return eow;
}

Token *token_from_word(const char *line, uint16_t *index_ptr)
{
    uint8_t i = *index_ptr;
    uint8_t j = 0;
    char tmp[MAX_TOKEN_LEN];

    while (!is_eow(line[i])) {
        tmp[j] = line[i];
        j++;
        i++;
    }

    *index_ptr = i - 1;

    Token *t = malloc(sizeof *t);

    t->size = j + 1;
    t->str = malloc(t->size * sizeof(char));

    for (uint8_t k = 0; k < j; k++) {
        t->str[k] = tmp[k];
    }
    t->str[j] = '\0';

    return t;
}

int8_t is_eol(char c)
{
    int8_t eol = 0;
    static const char eolchars[] = {0, 10, 13, 59};

    for (uint8_t i = 0; i < 4; i++) {
        if (c == eolchars[i])
            eol++;
    }
    return eol;
}

TokenList *tokenize(const char *line)
{
    TokenList *tl = malloc(sizeof *tl);
    tl->size = 0;
    tl->arr = malloc(sizeof(Token *));

    int8_t need_token = 0;
    int8_t is_sep = 0;
    int8_t is_quote = 0;
    int8_t prev_is_sep = 1;

    for (uint16_t i = 0; !is_eol(line[i]); i++) {
        is_sep = (line[i] == ' ' || line[i] == '\t' || line[i] == ',');
        is_quote = (line[i] == '\"');
        
        if (is_quote) {
            i++;
            tl->arr[tl->size] = token_from_string(line, &i);
            tl->size++;
            need_token = 1;
        } else if (prev_is_sep && !is_sep) {
            tl->arr[tl->size] = token_from_word(line, &i);
            tl->size++;
            is_sep = 1;
            need_token = 1;
        }

        if (need_token) {
            tl->arr = realloc(tl->arr, (tl->size + 1) * sizeof(Token *));
            need_token = 0;
        }
        prev_is_sep = is_sep;
    }
    return tl;
}

void free_token(Token *t)
{
    if (t) {
        if (t->str) {
            free(t->str);
        }
        free(t);
    }
}

void free_token_list(TokenList *tl)
{
    if (!tl) {
        return;
    }
    if (tl->arr) {
        for (uint8_t i = 0; i < tl->size; i++) {
            if (tl->arr[i]->size) {
                free_token(tl->arr[i]);
            }
        }
        free(tl->arr);
    }
    free(tl);
}

typedef struct line {
    TokenList *toks;
    uint32_t lnum;
} Line;

typedef struct linelist {
    Line **arr;
    uint32_t size;
    uint32_t cap;
} LineList;

#define LINELIST_DEFAULT_SIZE 200

LineList *create_line_list(void)
{
    LineList *ll = malloc(sizeof *ll);

    ll->cap = LINELIST_DEFAULT_SIZE;
    ll->size = 0;
    ll->arr = malloc(ll->cap * sizeof(Line *));

    return ll;
}

void add_line(LineList *ll, TokenList *tl, uint32_t lnum)
{
    if (ll->size == ll->cap) {
        ll->cap *= 2;
        ll->arr = realloc(ll->arr, ll->cap * sizeof(Line *));
    }

    ll->arr[ll->size] = malloc(sizeof(Line));
    ll->arr[ll->size]->toks = tl;
    ll->arr[ll->size]->lnum = lnum;

    ll->size++;
}

Line *get_line(LineList *ll, uint32_t idx)
{
    if (idx >= ll->size) {
        return NULL;
    }
    return ll->arr[idx];
}

void free_line_list(LineList *ll)
{
    if (!ll) {
        return;
    }
    for (uint32_t i = 0; i < ll->size; i++) {
        free_token_list(ll->arr[i]->toks);
        free(ll->arr[i]);
    }
    free(ll->arr);
    free(ll);
}

typedef struct outputbuffer {
    uint16_t *addrs;
    uint16_t *instrs;
    uint32_t *lnums;
    uint16_t size;
    uint16_t cap;
} OutputBuffer;

#define OUTPUT_BUFFER_DEFAULT_SIZE 128

OutputBuffer *create_output_buffer(void)
{
    OutputBuffer *ob = malloc(sizeof *ob);

    ob->size = 0;
    ob->cap = OUTPUT_BUFFER_DEFAULT_SIZE;

    ob->addrs = malloc(ob->cap * sizeof(uint16_t));
    ob->instrs = malloc(ob->cap * sizeof(uint16_t));
    ob->lnums = malloc(ob->cap * sizeof(uint32_t));

    return ob;
}

void add_to_output_buffer(OutputBuffer *ob, uint16_t addr, uint16_t instr, uint32_t lnum)
{
    if (ob->size == ob->cap) {
        ob->cap *= 2;
        ob->addrs = realloc(ob->addrs, ob->cap * sizeof(uint16_t));
        ob->instrs = realloc(ob->instrs, ob->cap * sizeof(uint16_t));
        ob->lnums = realloc(ob->lnums, ob->cap * sizeof(uint32_t));
    }

    ob->addrs[ob->size] = addr;
    ob->instrs[ob->size] = instr;
    ob->lnums[ob->size] = lnum;

    ob->size++;
}

void free_output_buffer(OutputBuffer *ob)
{
    if (!ob) {
        return;
    }
    if (ob->addrs) {
        free(ob->addrs);
    }
    if (ob->instrs) {
        free(ob->instrs);
    }
    if (ob->lnums) {
        free(ob->lnums);
    }
    free(ob);
}

typedef struct filelist {
    FILE *src;
    FILE *sym;
    FILE *obj;
    FILE *lst;
} FileList;

int8_t check_extension(char *filename, const char *ext)
{
    char *s = strrchr(filename, '.');
    if (!s) {
        return 0;
    }
    return !strcmp(s, ext);
}

void replace_extension(char *filename, const char *ext)
{
    char *s = strrchr(filename, '.');
    for (int i = 0; s[i] != '\0'; i++) {
        s[i] = ext[i];
    }
}

uint8_t open_file_list(FileList *fl, char *filename)
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

void close_file_list(FileList *fl)
{
    if (!fl) {
        return;
    }
    if (fl->src) {
        fclose(fl->src);
    }
    if (fl->sym) {
        fclose(fl->sym);
    }
    if (fl->obj) {
        fclose(fl->obj);
    }
    if (fl->lst) {
        fclose(fl->lst);
    }
}

uint8_t clean_output(char *filename)
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

void write_object(OutputBuffer *ob, FILE *obj)
{
    uint16_t byte_cnt = ob->size;
    uint8_t tmp[byte_cnt];

    for (uint16_t i = 0; i < ob->size; i++) {
        tmp[i * 2] = ob->instrs[i] >> 8;        // high byte
        tmp[i * 2 + 1] = ob->instrs[i] & 0xff;  // low byte
    }

    fwrite(tmp, sizeof(uint8_t), byte_cnt, obj);
}

void write_listing(OutputBuffer *ob, FILE *lst, FILE *src)
{
    static const char *bin[16] = {
        "0000", "0001", "0010", "0011",
        "0100", "0101", "0110", "0111",
        "1000", "1001", "1010", "1011",
        "1100", "1101", "1110", "1111"
    };

    #define binseq(x) \
        bin[x >> 12], \
        bin[(x >> 8) & 0xf], \
        bin[(x >> 4) & 0xf], \
        bin[x & 0xf]

    char line[MAX_LINE_LEN + 1];
    uint16_t idx = 0; // (doubt)
    uint16_t lim = ob->size;

    fseek(src, 0, SEEK_SET);

    fprintf(lst, "  Addr  |  Hex  |       Bin        | Line |  Source\n");

    for (uint32_t ln = 1; ; ln++) {
        if (!fgets(line, MAX_LINE_LEN + 1, src)) {
            break;
        }

        if (idx != lim && ob->lnums[idx] == ln) {
            uint16_t instr = ob->instrs[idx];
            uint16_t addr = ob->addrs[idx];
            fprintf(lst, " x%04X  | x%04X | %s%s%s%s ", addr, instr, binseq(instr));
            idx++;
        } else {
            fprintf(lst, "        |       |                  ");
        }

        fprintf(lst, "| %4d | %s", ln, line);

        while (idx != lim && ob->lnums[idx] == ln) {
            uint16_t instr = ob->instrs[idx];
            fprintf(lst, "        | x%04X | %s%s%s%s |      |\n", instr, binseq(instr));
            idx++;
        }
    }
}

typedef struct symbol {
    Token *sym;
    uint16_t addr;
} Symbol;

typedef struct symboltable {
    Symbol **arr;
    uint32_t size;
    uint32_t cap;
} SymbolTable;

#define SYMBOL_TABLE_DEFAULT_SIZE 8

void add_symbol(SymbolTable *st, Token *label, uint16_t addr)
{
    if (st->size == st->cap) {
        st->cap *= 2;
        st->arr = realloc(st->arr, st->cap * sizeof(Symbol *));
    }

    st->arr[st->size] = malloc(sizeof(Symbol));

    st->arr[st->size]->label = malloc(sizeof(Token));
    copy_token(st->arr[st->size]->label, label);
    st->arr[st->size]->addr = addr;

    st->size++;
}

SymbolTable *create_symbol_table(void)
{
    SymbolTable *st = malloc(sizeof(*st));

    st->size = 0;
    st->cap = SYMBOL_TABLE_DEFAULT_SIZE;
    st->arr = malloc(st->cap * sizeof(Symbol *));

    return st;
}

void free_symbol_table(SymbolTable *st)
{
    if (!st) {
        return;
    }

    for (uint16_t i = 0; i < st->size; i++) {
        free_token(st->arr[i]->sym);
        free(st->arr[i]);
    }
    free(st->arr);
    free(st);
}

uint16_t symbol_address(SymbolTable *st, Token *t)
{
    for (uint16_t i = 0; i < st->size; i++) {
        if (!strcmp(st->arr[i]->sym->str, t->str)) {
            return st->arr[i]->addr; 
        }
    }
    return 0;
}

Token *exist_symbol(SymbolTable *st, uint16_t addr)
{
    for (uint16_t i = 0; i < st->size; i++) {
        if (st->arr[i]->addr == addr) {
            return st->arr[i]->sym;
        }
    }
    return NULL;
}

enum Opcode {
    BR,    ADD,   LD,    ST,
    JSR,   AND,   LDR,   STR,
    RTI,   NOT,   LDI,   STI,
    JMP,   NVLD,  LEA,   TRAP,
    TRAPS, JSRR,  RET
};

int8_t is_register(Token *t)
{
    const char *regs[] = {
        "R0", "r0",
        "R1", "r1",
        "R2", "r2",
        "R3", "r3",
        "R4", "r4",
        "R5", "r5",
        "R6", "r6",
        "R7", "r7"
    };

    int8_t ret = -1;

    for (int8_t i = 0; i < 16; i++) {
        if (!strcmp(t->str, regs[i])) {
            ret = i / 2;
            break;
        }
    }

    return ret;
}

int8_t is_branch(Token *t)
{
    static const char *brs[] = {
        "BR", "BR", "br",
        "BRP", "BRp", "brp",
        "BRZ", "BRz", "brz",
        "BRZP", "BRzp", "brzp",
        "BRN", "BRn", "brn",
        "BRNP", "BRnp", "brnp",
        "BRNZ", "BRnz", "brnz",
        "BRNZP", "BRnzp", "brnzp"
    };

    int8_t ret = -1;

    for (int8_t i = 0; i < 24; i++) {
        if (!strcmp(t->str, brs[i])) {
            ret = i / 3;
            break;
        }
    }

    ret = (ret == 0) ? 7 : ret;
    return ret;
}

int8_t is_trap(Token *t)
{
    static const char *traps[] = {
        "GETC", "getc",
        "OUT", "out",
        "PUTS", "puts",
        "IN", "in",
        "PUTSP", "putsp",
        "HALT", "halt",
        "TRAP", "trap"
    };

    int8_t ret = -1;

    for (int8_t i = 0; i < 14; i++) {
        if (!strcmp(t->str, traps[i])) {
            ret = i / 2;
            break;
        }
    }

    if (ret == -1) {
        return -1;
    } else if (ret == 6) {
        return 0;
    }
    return 32 + ret;
}

int8_t is_opcode(Token *t)
{
    const char *ops[] = {
        "BR", "br",
        "ADD", "add",
        "LD", "ld",
        "ST", "st",
        "JSR", "jsr",
        "AND", "and",
        "LDR", "ldr",
        "STR", "str",
        "RTI", "rti",
        "NOT", "not",
        "LDI", "ldi",
        "STI", "sti",
        "JMP", "jmp",
        "", "",
        "LEA", "lea",
        "TRAP", "trap"
    };

    int8_t ret = -1;

    for (int8_t i = 0; i < 32; i++) {
        if (!strcmp(t->str, ops[i])) {
            ret = i / 2;
            break;
        }
    }

    if (ret < 0) {
        if (is_branch(t) >= 0) {
            ret = BR;
        } else if (is_trap(t) >= 0) {
            ret = TRAPS;
        } else if (!strcmp(t->str, "JSRR") || !strcmp(t->str, "jsrr")) {
            ret = JSRR;
        } else if (!strcmp(t->str, "RET") || !strcmp(t->str, "ret")) {
            ret = RET;
        }
    } else if (ret == NVLD) {
        ret = -1;
    }

    return ret;
}


uint8_t is_valid_label(Token *t)
{
    char *s = t->str;
    if (!isalpha(s[0]) && s[0] != '_') {
        return 0;
    }

    for (uint16_t i = 0; str[i] != '\0'; i++) {
        if (!isalnum(s[i]) && s[i] != '_') {
            return 0;
        }
    }

    if (is_register(t) != -1 || is_opcode(t) != -1) {
        return 0;
    }
    return 1;
}