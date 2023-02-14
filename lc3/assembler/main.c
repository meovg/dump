#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_TOKEN_NUM 7
#define MAX_TOKEN_LEN 1025
#define MAX_LINE_LEN 7175

typedef struct {
    char *str;
    uint16_t size;
} Token;

typedef struct {
    Token **arr;
    uint8_t size;
} TokenList;

char get_escape_value(char c)
{
    static const char esc_chrs[] = {
        '\'', '\"', '\?', '\\',
        'a', 'b', 'f', 'n', 'r', 't', 'v'
    };
    static const char esc_vals[] = {
        0x27, 0x22, 0x3F, 0x5C,
        0x07, 0x08, 0x0C, 0x0A, 0x0D, 0x09, 0x0B
    };

    for (uint8_t i = 0; i < 11; i++) {
        if (c == esc_chrs[i]) {
            return esc_vals[i];
        }
    }

    return c;
}

void copy_token(Token *dst, const Token *src)
{
    dst->size = src->size;
    dst->str = realloc(dst->str, dst->size * sizeof(char));
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
    static const char eow_chrs[] = {
        0, 13, '\t', '\n', ' ', ',', ';'
    };

    for (uint8_t i = 0; i < 7; i++) {
        if (c == eow_chrs[i]) {
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
    static const char eol_chrs[] = {0, 10, 13, 59};

    for (uint8_t i = 0; i < 4; i++) {
        if (c == eol_chrs[i]) {
            eol++;
        }
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
            tl->arr = realloc(tl->arr,
                    (tl->size + 1) * sizeof(Token *));
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

typedef struct {
    TokenList *toks;
    uint32_t lno;
} Line;

typedef struct {
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

void add_line(LineList *ll, TokenList *tl, uint32_t lno)
{
    if (ll->size == ll->cap) {
        ll->cap *= 2;
        ll->arr = realloc(ll->arr, ll->cap * sizeof(Line *));
    }

    ll->arr[ll->size] = malloc(sizeof(Line));
    ll->arr[ll->size]->toks = tl;
    ll->arr[ll->size]->lno = lno;

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

typedef struct {
    uint16_t *addrs;
    uint16_t *instrs;
    uint32_t *lnos;
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
    ob->lnos = malloc(ob->cap * sizeof(uint32_t));

    return ob;
}

void add_to_output_buffer(
    OutputBuffer *ob,
    uint16_t addr,
    uint16_t instr,
    uint32_t lno)
{
    if (ob->size == ob->cap) {
        ob->cap *= 2;
        ob->addrs = realloc(ob->addrs, ob->cap * sizeof uint16_t);
        ob->instrs = realloc(ob->instrs, ob->cap * sizeof uint16_t);
        ob->lnos = realloc(ob->lnos, ob->cap * sizeof uint32_t);
    }

    ob->addrs[ob->size] = addr;
    ob->instrs[ob->size] = instr;
    ob->lnos[ob->size] = lno;

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
    if (ob->lnos) {
        free(ob->lnos);
    }
    free(ob);
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
        printf("Couldn't open assembly file: %s!\n", filename);
        err++;
    }

    replace_extension(filename, ".sym");
    fl->sym = fopen(filename, "w");
    if (fl->sym == NULL) {
        printf("Couldn't create symbol table file: %s!\n", filename);
        err++;
    }

    replace_extension(filename, ".obj");
    fl->obj = fopen(filename, "w");
    if (fl->obj == NULL) {
        printf("Couldn't create object file: %s!\n", filename);
        err++;
    }

    replace_extension(filename, ".lst");
    fl->lst = fopen(filename, "w");
    if (fl->lst == NULL) {
        printf("Couldn't create listing file: %s!\n", filename);
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

static const char *binseq[16] = {
    "0000", "0001", "0010", "0011",
    "0100", "0101", "0110", "0111",
    "1000", "1001", "1010", "1011",
    "1100", "1101", "1110", "1111"
};

void write_listing(OutputBuffer *ob, FILE *lst, FILE *src)
{
    #define PRbinseq(x) \
        binseq[x >> 12], \
        binseq[(x >> 8) & 0xf], \
        binseq[(x >> 4) & 0xf], \
        binseq[x & 0xf]

    char line[MAX_LINE_LEN + 1];
    uint16_t idx = 0; // (doubt)
    uint16_t lim = ob->size;

    fseek(src, 0, SEEK_SET);

    fprintf(lst, "  Addr  |  Hex  |       Bin        | Line |  Source\n");

    for (uint32_t ln = 1; ; ln++) {
        if (!fgets(line, MAX_LINE_LEN + 1, src)) {
            break;
        }

        if (idx != lim && ob->lnos[idx] == ln) {
            uint16_t instr = ob->instrs[idx];
            uint16_t addr = ob->addrs[idx];
            fprintf(lst, " x%04X  | x%04X | %s%s%s%s ",
                    addr, instr, PRbinseq(instr));
            idx++;
        } else {
            fprintf(lst, "        |       |                  ");
        }

        fprintf(lst, "| %4d | %s", ln, line);

        while (idx != lim && ob->lnos[idx] == ln) {
            uint16_t instr = ob->instrs[idx];
            fprintf(lst, "        | x%04X | %s%s%s%s |      |\n",
                    instr, PRbinseq(instr));
            idx++;
        }
    }
}

typedef struct {
    Token *sym;
    uint16_t addr;
} Symbol;

typedef struct {
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
    OP_BR = 0,  // branch
    OP_ADD,     // add
    OP_LD,      // load
    OP_ST,      // store
    OP_JSR,     // jump register
    OP_AND,     // bitwise and
    OP_LDR,     // load register
    OP_STR,     // store register
    OP_RTI,     // return from interrupt (restricted)
    OP_NOT,     // bitwise not
    OP_LDI,     // load indirect
    OP_STI,     // store indirect
    OP_JMP,     // jump
    OP_RES,     // reserved
    OP_LEA,     // load effective address
    OP_TRAP,    // execute trap
    OP_TRAPS,   // TRAP
    OP_JSRR,    // JSR
    OP_RET,
};

enum PseudoOpcode {
    PS_ORIG,
    PS_END,
    PS_BLKW,
    PS_FILL,
    PS_STRINGZ,
};

int8_t is_register(Token *t)
{
    static const char *regs[] = {
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
    static const char *ops[] = {
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
            ret = OP_BR;
        } else if (is_trap(t) >= 0) {
            ret = OP_TRAPS;
        } else if (!strcmp(t->str, "JSRR") || !strcmp(t->str, "jsrr")) {
            ret = OP_JSRR;
        } else if (!strcmp(t->str, "RET") || !strcmp(t->str, "ret")) {
            ret = OP_RET;
        }
    } else if (ret == NVLD) {
        ret = -1;
    }

    return ret;
}

int8_t is_pseudo(Token *t)
{
    static const char *pseudos[] = {
        ".ORIG", ".orig",
        ".END", ".end",
        ".BLKW", ".blkw",
        ".FILL", ".fill",
        ".STRINGZ", ".stringz"
    }

    int8_t ret = -1;

    for (int8_t i = 0; i < 10; i++) {
        if (!strcmp(t->str, pseudos[i])) {
            ret = i / 2;
            break;
        }
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

enum OffsetType {
    OFF_RES,
    OFF_HEX,
    OFF_BIN,
    OFF_DEC,
};

int16_t twocomp(const char *s)
{
    static const int N = 16;
    static char seq[17];

    memset(seq, 0, 17 * sizeof(char));

    int16_t rep = 0;
    size_t len = strlen(s);
    size_t i = len - 1, j = 0;

    while (j < len && j < N) {
        seq[j] = s[i] - '0';
        j++;
        i--;
    }
    for (i = 0; i <= N-2; i++) {
        rep += seq[i] * (1 << i);
    }
    rep -= seq[N-1] * (1 << (N-1));

    return rep;
}

int8_t hex_index(char c)
{
    static const char hex[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    for (int8_t i = 0; i < 16; i++) {
        if (i < 10 && c == hex[i]) {
            return i;
        } else if (i >= 10 && (c == hex[i] || c == hex[i] + 32)) {
            return i;
        }
    }
    return -1;
}

int16_t parse_hex(const char *s) {
    static const int N = 4;
    static char seq[5];
    static char bin[17];

    memset(seq, 0, 5 * sizeof(char));
    memset(bin, 0, 17 * sizeof(char));
    
    char *offset = strchr(s, 'x');
    if (offset == NULL) {
        offset = strchr(s, 'X');
    }
    offset++;
    size_t len = strlen(offset);
    size_t i = len - 1, j = 0;

    while (j < len && j < N) {
        seq[j] = (char)hex_index(offset[i]);
        j++;
        i--;
    }
    i = 0;
    char *tmp = bin + 16;
    while (i < N) {
        tmp -= 4 * sizeof(char);
        memcpy(tmp, binseq[seq[i]], 4 * sizeof(char));
        i++;
    }

    return twocomp(bin);
}

int16_t parse_bin(char *s) {
    char *offset = strchr(s, 'b');
    if (offset == NULL) {
        offset = strchr(s, 'B');
    }
    offset++;
    return twocomp(offset);
}

int16_t parse_dec(char *s) {
    int8_t neg = 0;
    char *offset = strchr(s, '#');

    if (offset == NULL) {
        offset = s;
    } else {
        offset++;
    }

    if (offset[0] == '-') {
        neg = 1;
        offset++;
    }

    int16_t rep = 0;
    size_t i = 0;

    while (offset[i] != '\0') {
        rep = rep * 10 + (offset[i] - '0');
        i++;
    }
    if (neg) {
        rep = -rep;
    }
    return rep;
}

int16_t parse_offset(uint8_t offset_type, Token *t)
{
    char *s = t->str;
    int16_t val = 0;

    if (offset_type == OFF_HEX) {
        val = parse_hex(s);
    } else if (offset_type == OFF_BIN) {
        val = parse_bin(s);
    } else if (offset_type == OFF_DEC) {
        val = parse_dec(s);
    }
    return val;
}

uint8_t offset_type(Token *t)
{
    char *s = t->str;
    char *offset;

    if ((offset = strchr(s, 'x')) || (offset = strchr(s, 'X'))) {
        offset++;
        for (; *offset; offset++) {
            if (offset_hex_index(*offset) == -1) {
                return OFF_RES;
            }
        }
        return OFF_HEX;
    } else if ((offset = strchr(s, 'b')) || (offset = strchr(s, 'B'))) {
        offset++;
        for (; *offset; offset++) {
            if (*offset != '0' && *offset != '1') {
                return OFF_RES;
            }
        }
        return OFF_BIN;
    } else if (s != NULL) {
        offset = strchr(s, '#');
        if (offset == NULL) {
            offset = s;
        } else {
            offset++;
        }

        if (*offset == '-') {
            offset++;
        }
        for (; *offset; offset++) {
            if (!isdigit(*offset)) {
                return OFF_RES;
            }
        }
        return OFF_DEC;
    } else {
        return OFF_RES;
    }
}

uint16_t find_orig(FileList *fl, uint32_t *lno)
{
    uint16_t orig = 0;
    char line[MAX_LINE_LEN + 1];
    uint32_t i;

    for (i = *ln; fgets(line, MAX_LINE_LEN + 1, fl->src) != NULL; i++) {
        TokenList *tokens = tokenize(line);
        if (tokens->size == 0) {
            free_token_list(tokens);
            continue;
        }

        int8_t pseudo_idx = is_pseudo(tokens->arr[0]);
        if (pseudo_idx == ORIG) {

        }
    }
}