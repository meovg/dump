// reference
// https://www.cs.utexas.edu/users/fussell/courses/cs310h/lectures/Lecture_10-310h.pdf
// https://people.cs.georgetown.edu/~squier/Teaching/HardwareFundamentals/LC3-trunk/docs/LC3-AssemblyManualAndExamples.pdf
// https://github.com/PaperFanz/laser

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

/* //////// Error and warning //////// */

// error count and warning count when assembler is running
static int32_t errcnt = 0;
static int32_t warncnt = 0;

// notify an error on the terminal
void error(uint32_t linenum, char *format, ...) {
    va_list args;

    errcnt++;
    va_start(args, format);
    printf("Error: %d: ", linenum);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

// notify a warning on the terminal
void warning(uint32_t linenum, char *format, ...) {
    va_list args;

    warncnt++;
    va_start(args, format);
    printf("Warning: %d: ", linenum);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

/* //////// Tokens and lexing helpers //////// */

#define MAX_TOKEN_NUM 7
#define MAX_TOKEN_LEN 1025
#define MAX_LINE_LEN 7175   // MAX_TOKEN_NUM * MAX_TOKEN_LEN

typedef struct {
    char *str;
    uint16_t size;
} Token;

char escape_value(char c) {
    static const char esc_chrs[] = {
        '\'', '\"', '\?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'
    };
    static const char esc_vals[] = {
        0x27, 0x22, 0x3F, 0x5C, 0x07, 0x08, 0x0C, 0x0A, 0x0D, 0x09, 0x0B
    };

    int8_t i;

    for (i = 0; i < 11; i++) {
        if (c == esc_chrs[i]) {
            return esc_vals[i];
        }
    }
    return c;
}

int8_t is_eow(char c) {
    static const char eow_chrs[] = {
        '\0', '\r', '\t', '\n', ' ', ',', ';'
    };

    int8_t eow = 0;
    uint8_t i;

    for (i = 0; i < 7; i++) {
        if (c == eow_chrs[i]) {
            eow++;
        }
    }
    return eow;
}

int8_t is_eol(char c) {
    static const char eol_chrs[] = {
        '\0', '\n', '\r', ';' // semicolon marks the start of a comment
    };

    int8_t eol = 0;
    uint8_t i;

    for (i = 0; i < 4; i++) {
        if (c == eol_chrs[i]) {
            eol++;
        }
    }
    return eol;
}

// get a token of substring until the end of a line or a quote
// used for lexing string literals 
Token *token_from_string(const char *str, uint16_t *p_idx) {
    uint16_t i = *p_idx;
    uint16_t j = 0;
    uint16_t k;
    Token *tok;
    static char tmp[MAX_TOKEN_LEN + 1];

    // load characters to be included in the token in a temporary array
    while (str[i] != '\0' && str[i] != '\"') {
        if (str[i] == '\\') {   // '\' marks an escape character 
            tmp[j] = escape_value(str[++i]);
            j++;
            i++;
        } else {
            tmp[j] = str[i];
            j++;
            i++;
        }
    }

    // index stays at the end of a line
    // or position of the closing quote so the quote is skipped later
    *p_idx = i;

    tok = malloc(sizeof *tok);
    tok->size = j + 1;
    tok->str = malloc(tok->size * sizeof(char));

    for (k = 0; k < j; k++) {
        tok->str[k] = tmp[k];
    }
    tok->str[j] = '\0';

    return tok;
}

// get a token of substring until a word separator is found
// used for lexing symbols, opcodes, psuedo-ops and operands...
Token *token_from_word(const char *str, uint16_t *p_idx) {
    uint8_t i = *p_idx;
    uint8_t j = 0;
    uint8_t k;
    Token *tok;
    static char tmp[MAX_TOKEN_LEN + 1];

    while (!is_eow(str[i])) {
        tmp[j] = str[i];
        j++;
        i++;
    }

    // index should be at the last character in a token
    // so that tokenize() can check if the next character is a separator
    *p_idx = i - 1;

    tok = malloc(sizeof *tok);
    tok->size = j + 1;
    tok->str = malloc(tok->size * sizeof(char));

    for (k = 0; k < j; k++) {
        tok->str[k] = tmp[k];
    }
    tok->str[j] = '\0';

    return tok;
}

// deallocate the token
void token_del(Token *tok) {
    if (tok) {
        if (tok->str) {
            free(tok->str);
        }
        free(tok);
    }
}

typedef struct {
    Token **tokens;     // array of tokens in an instruction line
    uint8_t tokcnt;     // token count of the line
    uint32_t lidx;      // the line number (index) in the assembly file
} Line;

// break down an instruction line into several chunks of strings (tokens)
Line *line_new(uint32_t lidx, const char *str) {
    int8_t need_token = 0;          // if more token is needed
    int8_t is_sep = 0;              // if current char is separator
    int8_t prev_is_sep = 1;         // if the previous character is a separator
    Line *ln = malloc(sizeof *ln);  // returning object consisting of lexed tokens 
    uint16_t i;

    ln->lidx = lidx;
    ln->tokcnt = 0;
    ln->tokens = malloc(sizeof(Token *));

    for (i = 0; !is_eol(str[i]); i++) {
        is_sep = (str[i] == ' ' || str[i] == '\t' || str[i] == ',');

        if (str[i] == '\"') {
            i++;
            ln->tokens[ln->tokcnt] = token_from_string(str, &i);
            ln->tokcnt++;
            need_token = 1;
        } else if (prev_is_sep && !is_sep) {
            ln->tokens[ln->tokcnt] = token_from_word(str, &i);
            ln->tokcnt++;
            is_sep = 1;
            need_token = 1;
        }

        // the limit is only 7 so no need to realloc twice the current capacity
        if (need_token) {
            ln->tokens = realloc(ln->tokens, (ln->tokcnt + 1) * sizeof(Token *));
            need_token = 0;
        }

        prev_is_sep = is_sep;
    }
    return ln;
}

// deallocate a Line object and the tokens contained inside
void line_del(Line *ln) {
    uint8_t i;

    if (!ln) {
        return;
    }

    for (i = 0; i < ln->tokcnt; i++) {
        if (ln->tokens[i]->size) {
            token_del(ln->tokens[i]);
        }
    }
    free(ln->tokens);
    free(ln);
}

// dynamic container of Line objects
typedef struct {
    Line **arr;
    uint32_t size;
    uint32_t cap;
} InputBuf;

#define INPUTBUF_DEFAULT_SIZE 200

InputBuf *inputbuf_new(void) {
    InputBuf *ibuf = malloc(sizeof *ibuf);

    ibuf->cap = INPUTBUF_DEFAULT_SIZE;
    ibuf->size = 0;
    ibuf->arr = malloc(ibuf->cap * sizeof(Line *));
    return ibuf;
}

void inputbuf_add(InputBuf *ibuf, Line *ln) {
    if (ibuf->size == ibuf->cap) {
        ibuf->cap *= 2;
        ibuf->arr = realloc(ibuf->arr, ibuf->cap * sizeof(Line *));
    }

    ibuf->arr[ibuf->size] = ln;
    ibuf->size++;
}

// retrieve a Line object based on the index
Line *inputbuf_get(InputBuf *ibuf, uint32_t idx) {
    if (idx >= ibuf->size) {
        return NULL;
    }
    return ibuf->arr[idx];
}

// deallocate Line list and the lines contained
void inputbuf_del(InputBuf *ibuf) {
    uint32_t i;

    if (!ibuf) {
        return;
    }
    for (i = 0; i < ibuf->size; i++) {
        line_del(ibuf->arr[i]);
    }
    free(ibuf->arr);
    free(ibuf);
}

/* //////// output buffer //////// */

// storing assembled machine codes and related infos to write into
// program's object file and listing file
typedef struct {
    uint16_t *instr;    // machine codes
    uint16_t *addr;     // addresses of machine codes
    uint32_t *lidx;     // indexes of instructions in the assembly
                        // file that map to respective machine codes
    uint16_t size;
    uint16_t cap;
} OutputBuf;

#define OUTPUTBUF_DEFAULT_SIZE 128

OutputBuf *outputbuf_new(void) {
    OutputBuf *obuf = malloc(sizeof *obuf);

    obuf->size = 0;
    obuf->cap = OUTPUTBUF_DEFAULT_SIZE;
    obuf->addr = malloc(obuf->cap * sizeof(uint16_t));
    obuf->instr = malloc(obuf->cap * sizeof(uint16_t));
    obuf->lidx = malloc(obuf->cap * sizeof(uint32_t));
    return obuf;
}

// add assembled instructions, address and linenum to output buffer
// to be used later on
void outputbuf_add(OutputBuf *obuf, uint16_t addr, uint16_t instr, uint32_t lidx) {
    if (obuf->size == obuf->cap) {
        obuf->cap *= 2;
        obuf->addr = realloc(obuf->addr, obuf->cap * sizeof(uint16_t));
        obuf->instr = realloc(obuf->instr, obuf->cap * sizeof(uint16_t));
        obuf->lidx = realloc(obuf->lidx, obuf->cap * sizeof(uint32_t));
    }

    obuf->addr[obuf->size] = addr;
    obuf->instr[obuf->size] = instr;
    obuf->lidx[obuf->size] = lidx;
    obuf->size++;
}

void outputbuf_del(OutputBuf *obuf) {
    if (!obuf) {
        return;
    }
    if (obuf->addr) {
        free(obuf->addr);
    }
    if (obuf->instr) {
        free(obuf->instr);
    }
    if (obuf->lidx) {
        free(obuf->lidx);
    }
    free(obuf);
}

/* //////// symbol table and helpers //////// */

typedef struct {
    Token *key;         // symbol (label)
    uint16_t addr;      // symbol's instruction address
} Symbol;

typedef struct {
    Symbol **arr;
    uint32_t size;
    uint32_t cap;
} SymTab;

#define SYMTAB_DEFAULT_SIZE 8

SymTab *symtab_new(void) {
    SymTab *st = malloc(sizeof(*st));

    st->size = 0;
    st->cap = SYMTAB_DEFAULT_SIZE;
    st->arr = malloc(st->cap * sizeof(Symbol *));
    return st;
}

void symtab_add(SymTab *st, Token *key, uint16_t addr) {
    Symbol *sym;

    if (st->size == st->cap) {
        st->cap *= 2;
        st->arr = realloc(st->arr, st->cap * sizeof(Symbol *));
    }
    st->arr[st->size] = malloc(sizeof(Symbol));

    // allocate and copy the contents from label token to sym->key
    sym = st->arr[st->size];
    sym->key = malloc(sizeof(Token));
    sym->key->size = key->size;
    sym->key->str = malloc(key->size * sizeof(char));
    memcpy(sym->key->str, key->str, key->size);
    sym->addr = addr;

    st->size++;
}

void symtab_del(SymTab *st) {
    uint16_t i;

    if (!st) {
        return;
    }
    for (i = 0; i < st->size; i++) {
        token_del(st->arr[i]->key);
        free(st->arr[i]);
    }
    free(st->arr);
    free(st);
}

// get the instruction address of a symbol from the symbol table
uint16_t symtab_get(SymTab *st, Token *key) {
    uint16_t i;

    for (i = 0; i < st->size; i++) {
        if (!strcmp(st->arr[i]->key->str, key->str)) {
            return st->arr[i]->addr;
        }
    }
    return 0;
}

// check if an instruction address has a symbol referring to it
// or return NULL
Token *symtab_exists(SymTab *st, uint16_t addr) {
    uint16_t i;

    for (i = 0; i < st->size; i++) {
        if (st->arr[i]->addr == addr) {
            return st->arr[i]->key;
        }
    }
    return NULL;
}

/* //////// Files output //////// */

int8_t extension_check(char *filename, const char *ext) {
    char *s = strrchr(filename, '.');
    if (!s) {
        return 0;
    }
    return !strcmp(s, ext);
}

void extension_replace(char *filename, const char *ext) {
    char *s = strrchr(filename, '.');
    for (int i = 0; s[i] != '\0'; i++) {
        s[i] = ext[i];
    }
}

// group of files to work with
typedef struct {
    FILE *src;  // assembly file (read plain text)
    FILE *sym;  // symbol table file (write plain text)
    FILE *obj;  // machine code file (write binary)
    FILE *lst;  // listing file (write plain text)
} FileList;

// this function is simply initialization so allocate FileList object yourself, thanks :)
void filelist_open(FileList *fl, char *filename) {
    uint8_t err = 0;

    fl->src = fopen(filename, "r");
    if (fl->src == NULL) {
        error(0, "Couldn't open assembly file: %s", filename);
    }

    extension_replace(filename, ".sym");
    fl->sym = fopen(filename, "w");
    if (fl->sym == NULL) {
        error(0, "Couldn't create symbol table file: %s", filename);
    }

    extension_replace(filename, ".obj");
    fl->obj = fopen(filename, "wb");  // binary file
    if (fl->obj == NULL) {
        error(0, "Couldn't create object file: %s", filename);
    }

    extension_replace(filename, ".lst");
    fl->lst = fopen(filename, "w");
    if (fl->lst == NULL) {
        error(0, "Couldn't create listing file: %s", filename);
    }

    extension_replace(filename, ".asm");
}

void filelist_close(FileList *fl) {
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

// delete unused output files, used when errors occur during assembling process
void clean_output(char *filename) {
    static const char *exts[] = {".sym", ".obj", ".lst"};
    uint8_t i;

    printf("\nCleaning up output files...\n");
    for (i = 0; i < 3; i++) {
        extension_replace(filename, exts[i]);
        if (remove(filename)) {
            warning(0, "Couldn't delete %s", filename);
        }
    }
    extension_replace(filename, ".asm");
}

// write assembled machine instructions to .obj file
void write_obj(OutputBuf *obuf, FILE *obj) {
    uint16_t i;
    uint16_t tmp;   // store instruction for each loop

    // (the struggle to swap the byte order of machine instructions
    // on both the assembler and simulator just because LC-3 uses big endian)
    // i guess object file is written last then...
    for (i = 0; i < obuf->size; i++) {
        tmp = obuf->instr[i];
        obuf->instr[i] = (tmp << 8) | (tmp >> 8);
    }
    fwrite(obuf->instr, sizeof(uint16_t), obuf->size, obj);
}

// write the symbol table to .sym file
// symbols are listed in the second column while their addr are in the first
void write_sym(SymTab *st, FILE *sym) {
    uint32_t i;

    fprintf(sym, "  Addr  |  Symbol\n");
    for (i = 0; i < st->size; i++) {
        fprintf(sym, " x%04X  |  %s\n", st->arr[i]->addr, st->arr[i]->key->str);
    }
}

static const char *binseq[16] = {
    "0000", "0001", "0010", "0011",
    "0100", "0101", "0110", "0111",
    "1000", "1001", "1010", "1011",
    "1100", "1101", "1110", "1111"
};

// write listing table to .lst file
// each column of table specifies assembled machine code addr in hex (1)
// instructions in hex (2) and binary (3), the index (4) and content (5)
// of respective source codes in assembly file
//
// note that a line in source code can map to 0, 1 or many machine instructions

// sequence used to print instructions in binary
#define PRbinseq(x) \
    binseq[x >> 12], \
    binseq[(x >> 8) & 0xf], \
    binseq[(x >> 4) & 0xf], \
    binseq[x & 0xf]

void write_lst(OutputBuf *obuf, FILE *lst, FILE *src) {
    static char line[MAX_LINE_LEN + 1]; // store the line read from assembly file
    uint16_t i = 1;     // outbuf index, skip 0 since it only contains .orig address
    uint32_t lidx;      // assembly file line index
    uint16_t instr;     // store instruction for each loop
    uint16_t addr;      // store address for each loop

    fseek(src, 0, SEEK_SET);
    fprintf(lst, "  Addr  |  Hex  |       Bin        | Line |  Source\n");

    for (lidx = 1; fgets(line, MAX_LINE_LEN + 1, src); lidx++) {
        // write first 3 columns if any matched instruction is found or leave them blank
        if (i != obuf->size && obuf->lidx[i] == lidx) {
            instr = obuf->instr[i];
            addr = obuf->addr[i];
            fprintf(lst, " x%04X  | x%04X | %s%s%s%s ", addr, instr, PRbinseq(instr));
            i++;
        } else {
            fprintf(lst, "        |       |                  ");
        }

        // write respective line in source code
        // fgets reads newline as well so there's no need to add \n
        fprintf(lst, "| %4d | %s", lidx, line);

        // write other machine codes in case an assembly instruction maps to multiple machine codes (.STRINGZ and .BLKW)
        while (i != obuf->size && obuf->lidx[i] == lidx) {
            instr = obuf->instr[i];
            fprintf(lst, "        | x%04X | %s%s%s%s |      |\n", instr, PRbinseq(instr));
            i++;
        }
    }
}

#undef PRbinseq

/* //////// Token validation and evaluation //////// */

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
    PS_ORIG,    // notify where program code begins
    PS_END,     // tell assembler to stop assemble
    PS_BLKW,    // reserve memory locations
    PS_FILL,    // place value at that line of code
    PS_STRINGZ, // place a string literal
};

// check if token is a register or return -1
int8_t is_register(Token *tok) {
    static const char *registers[] = {
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
    int8_t i;

    for (i = 0; i < 16; i++) {
        if (!strcmp(tok->str, registers[i])) {
            ret = i / 2;
            break;
        }
    }
    return ret;
}

// check if token is correct branch options or return the default BRnzp
int8_t is_branch(Token *tok) {
    static const char *br_options[] = {
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
    int8_t i;

    for (i = 0; i < 24; i++) {
        if (!strcmp(tok->str, br_options[i])) {
            ret = i / 3;
            break;
        }
    }
    ret = (ret == 0) ? 7 : ret;
    return ret;
}

// check if token is trapcode or return -1
int8_t is_trap(Token *tok) {
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
    int8_t i;

    for (i = 0; i < 14; i++) {
        if (!strcmp(tok->str, traps[i])) {
            ret = i / 2;
            break;
        }
    }
    if (ret == -1) {
        return -1;
    } else if (ret == 6) {
        return 0;
    }
    return 32 + ret; // defined trapvects are from 0x20 to 0x25
}

// check if token is an opcode and return its index or -1
int8_t is_opcode(Token *tok) {
    static const char *opcodes[] = {
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
    int8_t i;

    for (i = 0; i < 32; i++) {
        if (!strcmp(tok->str, opcodes[i])) {
            ret = i / 2;
            break;
        }
    }
    if (ret < 0) { // check if token is trapcode or opcode not defined above
        if (is_branch(tok) >= 0) {
            ret = OP_BR;
        } else if (is_trap(tok) >= 0) {
            ret = OP_TRAPS;
        } else if (!strcmp(tok->str, "JSRR") || !strcmp(tok->str, "jsrr")) {
            ret = OP_JSRR;
        } else if (!strcmp(tok->str, "RET") || !strcmp(tok->str, "ret")) {
            ret = OP_RET;
        }
    } else if (ret == OP_RES) {
        ret = -1;
    }
    return ret;
}

int8_t is_pseudo(Token *tok) {
    static const char *pseudos[] = {
        ".ORIG", ".orig",
        ".END", ".end",
        ".BLKW", ".blkw",
        ".FILL", ".fill",
        ".STRINGZ", ".stringz"
    };

    int8_t ret = -1;
    int8_t i;

    for (i = 0; i < 10; i++) {
        if (!strcmp(tok->str, pseudos[i])) {
            ret = i / 2;
            break;
        }
    }
    return ret;
}

// check if token is a valid symbol
// (first character is an alphabetical character or underscore, the rest are
// either alphabetical, underscore or a numeric digit;
// symbol must not be equal to reserved keywords: register, opcode, pseudo-ops...)
uint8_t is_valid_symbol(Token *tok) {
    char *s = tok->str;
    uint16_t i;

    if (!isalpha(s[0]) && s[0] != '_') {
        return 0;
    }
    for (i = 0; s[i] != '\0'; i++) {
        if (!isalnum(s[i]) && s[i] != '_') {
            return 0;
        }
    }
    // no need to check if it's a pseudo-op as the first character cannot be period
    return is_register(tok) == -1 && is_opcode(tok) == -1;
}

// literal (immediate) value that is hardcoded in the source file
// binary and hexadecimal literal must be represented in two's complement
// while literal in decimal can use the negative sign
enum LiteralType {
    LIT_RES,    // invalid type (reserved for the trolls)
    LIT_HEX,    // hexadecimal type
    LIT_BIN,    // binary type
    LIT_DEC,    // decimal type
};

// calculate the two's complement binary sequence
// refer to https://en.wikipedia.org/wiki/Two%27s_complement#Converting_from_two's_complement_representation
// for the formula
int16_t parse_two_complement(const char *s) {
    static const int N = 16;    // bit count limit
    static char seq[17];        // store valid binary sequence in char
    int16_t res = 0;            // return value
    size_t len = strlen(s);     // length of given binary sequence
    size_t i = len - 1;         // iterates through s
    size_t j = 0;               // iterates through seq

    // copy bit sequence from the bottom up for easier looking calculation
    while (j < len && j < N) {
        seq[j] = s[i] - '0';
        j++;
        i--;
    }

    for (j = 0; j <= N-2; j++) {
        res += seq[j] * (1 << j);
    }
    res -= seq[N-1] * (1 << (N-1));
    return res;
}

// get decimal value of hex digit
int8_t hex_index(char c) {
    static const char hex[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    int8_t i;

    for (i = 0; i < 16; i++) {
        if (i < 10 && c == hex[i]) {
            return i;
        } else if (i >= 10 && (c == hex[i] || c == hex[i] + 32)) {
            return i;
        }
    }
    return -1;
}

// calculate the two's complement literal represented in hex code
int16_t parse_hex(const char *s) {
    static const int N = 4;
    static char seq[5];     // stores the 4 bits to convert, just in case :)
    static char bin[17];    // binary sequence to be used in parse_two_complement()
    char *num = NULL;       // points to the numerical part of `s`
    size_t len;             // length of `num`
    size_t i;               // iterates through `num`
    size_t j;               // iterates through `seq`
    char *tmp;              // used in copying binary sequence

    memset(seq, 0, 5 * sizeof(char));
    memset(bin, 0, 17 * sizeof(char));

    // move to the start of actual number in literal
    num = strchr(s, 'x');
    if (num == NULL) {
        // a reminder for heathens that write in uppercase to pet your right shift key too
        num = strchr(s, 'X');
    }
    // skip that x too
    num++;

    len = strlen(num);
    i = len - 1;
    j = 0;
    while (j < len && j < N) {
        seq[j] = hex_index(num[i]);
        j++;
        i--;
    }

    // transcode into binary sequence
    j = 0;
    tmp = bin + 16;
    while (j < N) {
        tmp -= 4 * sizeof(char);
        memcpy(tmp, binseq[seq[j]], 4 * sizeof(char));
        j++;
    }
    return parse_two_complement(bin);
}

int16_t parse_bin(char *s) {
    char *num = strchr(s, 'b');
    if (num == NULL) {
        num = strchr(s, 'B');
    }
    num++;
    return parse_two_complement(num);
}

int16_t parse_dec(char *s) {
    int8_t neg = 0;             // if the literal is negative
    int16_t res = 0;            // returned value
    char *num = strchr(s, '#'); // numerical part of literal
    size_t i = 0;               // iterates through `num`

    if (num == NULL) {
        num = s;
    } else {
        num++;
    }
    if (num[0] == '-') {
        neg = 1;
        num++;
    }
    while (num[i] != '\0') {
        res = res * 10 + (num[i] - '0');
        i++;
    }
    return neg ? -res : res;
}

int16_t parse_literal(uint8_t type, Token *tok) {
    char *s = tok->str;
    int16_t val = 0;

    if (type == LIT_HEX) {
        val = parse_hex(s);
    } else if (type == LIT_BIN) {
        val = parse_bin(s);
    } else if (type == LIT_DEC) {
        val = parse_dec(s);
    }
    return val;
}

// return the literal type of the presumably "literal" token
uint8_t literal_type(Token *tok) {
    char *s = tok->str;
    char *lit;

    // hexadecimal type: x$$$$ or X$$$$ format (no negative sign)
    // $ in range [0..9, A..F]
    if ((lit = strchr(s, 'x')) || (lit = strchr(s, 'X'))) {
        lit++;
        while (*lit != '\0') {
            if (hex_index(*lit) == -1) {
                return LIT_RES;
            }
            lit++;
        }
        return LIT_HEX;

    // binary type: b$$...$ or B$$...$ format (no negative sign)
    // $ is either 0 or 1
    } else if ((lit = strchr(s, 'b')) || (lit = strchr(s, 'B'))) {
        lit++;
        while (*lit != '\0') {
            if (*lit != '0' && *lit != '1') {
                return LIT_RES;
            }
            lit++;
        }
        return LIT_BIN;

    // decimal type: #$$$..$ or $$$..$ format (1 negative sign allowed)
    } else if (s != NULL) {
        lit = strchr(s, '#');

        if (lit == NULL) {
            lit = s;
        } else {
            lit++;
        }
        if (*lit == '-') {
            lit++;
        }
        while (*lit != '\0') {
            if (!isdigit(*lit)) {
                return LIT_RES;
            }
            lit++;
        }
        return LIT_DEC;
    }
    return LIT_RES;
}

/* //////// actual assembling routines //////// */

// find pseudo-op .ORIG and get the address to the start of the code
uint16_t find_orig(
    FileList *fl,       // group of working files
    InputBuf *ibuf,     // input buffer containing lexed assembly instructions
    OutputBuf *obuf,    // output buffer containing machine codes
    uint32_t *p_lidx    // points to the current line index in the routine that calls this function 
) {
    static char str[MAX_LINE_LEN + 1]; // char array to read lines from files
    uint16_t orig = 0;  // returned value, the starting address of the program  
    uint32_t i;         // acts as line index for *p_lidx
    Line *ln;           // points lexed line object

    for (i = *p_lidx; fgets(str, MAX_LINE_LEN + 1, fl->src) != NULL; i++) {
        int8_t pseudo_idx;  // stores the pseudo enum value 

        ln = line_new(i, str);
        if (ln->tokcnt == 0) {
            line_del(ln);
            continue;
        }

        pseudo_idx = is_pseudo(ln->tokens[0]);
        if (pseudo_idx == PS_ORIG) {    // .orig found 
            orig = parse_literal(LIT_HEX, ln->tokens[1]);
            inputbuf_add(ibuf, ln);
            outputbuf_add(obuf, 0, orig, i);
            break;
        } else {
            warning(i, "%s instruction ignored", ln->tokens[0]);
        }
    }
    *p_lidx = i;
    return orig;
}

const uint8_t opcode_operand_cnt[] = {
    1,  // BR
    3,  // ADD
    2,  // LD
    2,  // ST
    1,  // JSR
    3,  // AND
    3,  // LDR
    3,  // STR
    0,  // RTI
    2,  // NOT
    2,  // LDI
    2,  // STI
    1,  // JMP
    0,  // RES
    2,  // LEA
    1,  // TRAP
    0,  // TRAPS
    1,  // JSRR
    0   // RET
};

const uint8_t pseudo_operand_cnt[] = {
    1,  // .ORIG
    0,  // .END
    1,  // .BLKW
    1,  // .FILL
    1   // .STRINGZ
};

// identify symbols and put them into symbol table
// also check for valid opcodes, pseudo-ops, and operand counts
uint8_t validate_line(
    InputBuf *ibuf,     // input buffer with list of lexed lines
    SymTab *st,         // symbol table
    Line *ln,           // line object
    uint16_t *p_addr    // current instruction address
) {
    uint8_t end_found = 0;      // return value, set to 1 if .END is found
    uint32_t lidx = ln->lidx; // current line number
    char *tmp;
    int8_t opcode = -1;         // opcode index defined in some enum idk
    int8_t pseudo = -1;         // same thing but for pseudo-ops
    int8_t operand_exp = -1;    // expected operand counts
    int8_t operand_cnt = 0;     // actual operand counts
    Token *tok;
    uint8_t i;

    for (i = 0; i < ln->tokcnt; i++) {
        tok = ln->tokens[i];

        if (i == 0 && is_valid_symbol(tok)) {
            // check if any symbol also points to the current address
            Token *dupe = symtab_exists(st, *p_addr);

            if (dupe == NULL) {
                symtab_add(st, tok, *p_addr);
            } else {
                warning(lidx, "Another symbol is pointing to address %04x: %s", *p_addr, dupe->str);
            }
            operand_exp = 0;
            tmp = tok->str;
        } else if ((opcode = is_opcode(tok)) != -1) {
            operand_exp = opcode_operand_cnt[opcode];
            tmp = tok->str;
            *p_addr += 1;
        } else if ((pseudo = is_pseudo(tok)) != -1) {
            Token *next;

            operand_exp = pseudo_operand_cnt[pseudo];
            tmp = tok->str;
            if (pseudo == PS_END) { // .END is found, leave now
                end_found = 1;
                break;
            }
            // this is already an "operand not found" error but
            // let's propagate that to operand check at the end of the function
            if (i + 1 >= ln->tokcnt) {
                break;
            }

            next = ln->tokens[i + 1];
            if (pseudo == PS_STRINGZ) {
                // offset is the length of string literal
                *p_addr += next->size;
                i++;
                operand_cnt++;
            } else if (pseudo == PS_BLKW) {
                // offset is the literal
                uint8_t type = literal_type(next);
                if (type != LIT_RES) {
                    *p_addr += parse_literal(type, next);
                }
            } else if (pseudo == PS_FILL) {
                *p_addr += 1;
            } else {
                warning(lidx, "%s ignored", tok->str);
            }
        } else if (is_register(tok) != -1 || literal_type(tok) != LIT_RES || is_valid_symbol(tok)) {
            operand_cnt++;
        } else {
            error(lidx, "Unknown token %s", tok->str);
        }
    }

    if (operand_exp == -1) {
        // nothing, just to make it readable
    } else if (operand_cnt > operand_exp) {
        warning(lidx, "%s has more operands than expected", ln->tokens[0]->str);
    } else if (operand_cnt < operand_exp) {
        error(lidx, "%s has not enough operands", ln->tokens[0]->str);
    }

    // add current line to the line list so the program can move on from
    // reading this forsaken source code file
    inputbuf_add(ibuf, ln);

    return end_found;
}

const int8_t opcode_val[] = {
    0,  // BR
    1,  // ADD
    2,  // LD
    3,  // ST
    4,  // JSR
    5,  // AND
    6,  // LDR
    7,  // STR
    8,  // RTI
    9,  // NOT
    10, // LDI
    11, // STI
    12, // JMP
    13, // RES
    14, // LEA
    15, // TRAP
    15, // TRAPS -> TRAP
    4,  // JSRR -> JSR
    12  // RET -> JMP
};

// operand mask only shows which operands are required for an opcode
// and has nothing to do with translating to machine code
enum {
    DstSrc  = 1,
    Src1    = 1 << 1,
    Src2    = 1 << 2,
    PCoff9  = 1 << 3,
    PCoff11 = 1 << 4,
    Off6    = 1 << 5,
    Cond    = 1 << 6,
    Tvec8   = 1 << 7,
};

const uint8_t operand_mask[] = {
    Cond    | PCoff9,               // BR
    DstSrc  | Src1      | Src2,     // ADD
    DstSrc  | PCoff9,               // LD
    DstSrc  | PCoff9,               // ST
    PCoff11,                        // JSR
    DstSrc  | Src1      | Src2,     // AND
    DstSrc  | Src1      | Off6,     // LDR
    DstSrc  | Src1      | Off6,     // STR
    0,                              // RTI
    DstSrc  | Src1,                 // NOT
    DstSrc  | PCoff9,               // LDI
    DstSrc  | PCoff9,               // STI
    Src1,                           // JMP
    0,                              // RES
    DstSrc  | PCoff9,               // LEA
    Tvec8,                          // TRAP
    Tvec8,                          // TRAPS -> TRAP
    Src1,                           // JSRR
    Src1                            // RET -> JMP
};

#define in_range_5_bit(x)   ((-16 <= (x)) && ((x) <= 15))
#define in_range_6_bit(x)   ((-32 <= (x)) && ((x) <= 31))
#define in_range_9_bit(x)   ((-256 <= (x)) && ((x) <= 255))
#define in_range_11_bit(x)  ((-1024 <= (x)) && ((x) <= 1023))

// generate machine code based on assembly instruction
uint8_t generate_machine_code(
    OutputBuf *obuf,    // output buffer
    SymTab *st,         // symbol table
    Line *ln,           // lexed instruction line
    uint16_t *p_addr    // instruction address
) {
    uint8_t end_found = 0;          // return value, checks if .end is found to stop
    uint32_t lidx = ln->lidx;       // this instruction line index in assembly file 
    Token **toks = ln->tokens;      // list of tokens in line
    int8_t opcode = -1;             // opcode enum value
    int8_t pseudo = -1;             // pseudo enum value
    uint16_t sym_addr;              // symbol address

    // check symbol table for possible symbol found
    sym_addr = symtab_get(st, toks[0]);
    if (sym_addr) {
        if (ln->tokcnt == 1) {
            return 0;
        }
        toks++;
    }

    if ((opcode = is_opcode(toks[0])) != -1) {
        uint16_t ins = opcode_val[opcode] << 12;
        uint8_t opmask = operand_mask[opcode];

        // NOT opcode gets special treatment as its instruction has 6 lowest bits
        // filled with 1, aside from Dst and Src1
        if (opcode == OP_NOT) {
            ins |= 0b111111;
        }

        if (opmask & DstSrc) {
            int8_t reg = is_register(toks[1]);
            if (reg >= 0) {
                ins |= (reg << 9);
            } else {
                error(lidx, "%s is not a register", toks[1]->str);
            }
        }

        if (opmask & Src1) {
            int8_t reg = 0;
            int8_t rhs = 0;

            if (opcode == OP_RET) {
                reg = 7;
            } else if (opcode == OP_JMP || opcode == OP_JSRR) {
                rhs = 1;
            } else {
                rhs = 2;
            }

            if (reg == 7 || (reg = is_register(toks[rhs])) != -1) {
                ins |= (reg << 6);
            } else {
                error(lidx, "%s is not a register", toks[rhs]->str);
            }
        }

        // also handles Imm5 if token is not a register
        if (opmask & Src2) {
            int8_t reg = 0;
            uint8_t type = 0;

            if ((reg = is_register(toks[3])) != -1) {
                ins |= reg;
            } else if ((type = literal_type(toks[3])) != LIT_RES) {
                int16_t imm5 = parse_literal(type, toks[3]);

                if (!in_range_5_bit(imm5)) {
                    error(lidx, "Immediate %s cannot be represented in 5 bits", toks[3]->str);
                }

                // the 1 in the 5th indicates immediate mode
                // telling machine to inteprete the lowest 5 bits as literal
                ins |= (imm5 & 0b11111);
                ins |= (1 << 5);
            } else {
                error(lidx, "%s is not a valid operand for Src2", toks[3]->str);
            }
        }

        // no need to check since the default branch option is BRnzp
        if (opmask & Cond) {
            ins |= (is_branch(toks[0]) << 9);
        }

        if (opmask & PCoff9) {
            uint8_t type = 0;
            uint8_t rhs = opcode == OP_BR ? 1 : 2;
            int16_t off = 0;

            sym_addr = symtab_get(st, toks[rhs]);
            if (sym_addr) {
                off = sym_addr - (*p_addr + 1);
                if (!in_range_9_bit(off)) {
                    error(lidx, "PCoffset %04x cannot be represented in 9 bits", off);
                }
                ins |= (off & 0b111111111);
            } else if ((type = literal_type(toks[rhs])) != LIT_RES) {
                off = parse_literal(type, toks[rhs]);
                if (!in_range_9_bit(off)) {
                    error(lidx, "PCoffset %04x cannot be represented in 9 bits", off);
                }
                ins |= (off & 0b111111111);
            } else {
                error(lidx, "%s is not a valid operand for PCoff9", toks[rhs]->str);
            }
        }

        if (opmask & PCoff11) { // exclusive to JSR
            uint8_t type = 0;
            int16_t off = 0;

            ins |= (1 << 11);
            sym_addr = symtab_get(st, toks[1]);

            if (sym_addr) {
                off = sym_addr - (*p_addr + 1);

                if (!in_range_11_bit(off)) {
                    error(lidx, "PCoffset %04x cannot be represented in 11 bits", off);
                }
                ins |= (off & 0b11111111111);
            } else if ((type = literal_type(toks[1])) != LIT_RES) {
                off = parse_literal(type, toks[1]);

                if (!in_range_11_bit(off)) {
                    error(lidx, "PCoffset %04x cannot be represented in 11 bits", off);
                }
                ins |= (off & 0b11111111111);
            } else {
                error(lidx, "%s is not a valid operand for PCoff11", toks[1]->str);
            }
        }

        if (opmask & Off6) {
            uint8_t type = literal_type(toks[3]);
            int16_t off = 0;

            if (type != LIT_RES) {
                off = parse_literal(type, toks[3]);
                if (!in_range_6_bit(off)) {
                    error(lidx, "Offset %04x cannot be represented in 6 bits", off);
                }
                ins |= (off & 0b111111);
            } else {
                error(lidx, "%s is not a valid operand for Off6", toks[3]->str);
            }
        }

        // trapcode, validation is at validate_line() (inside is_opcode)
        if (opmask & Tvec8) {
            int16_t trapvect8 = is_trap(toks[0]);

            // case instruction is in `TRAP <trapvect8>` format
            if (trapvect8 == 0) {
                uint8_t type = literal_type(toks[1]);

                if (type != LIT_RES) {
                    trapvect8 = parse_literal(type, toks[1]);
                    if (trapvect8 < 0 || trapvect8 > 0xff) {
                        error(lidx, "%s is not a valid trapvector", toks[1]->str);
                    } else if (trapvect8 < 0x20 || trapvect8 > 0x25) {
                        warning(lidx, "Undefined trap routine %s ignored", toks[1]->str);
                    }
                } else {
                    error(lidx, "%s is not a valid trapvector", toks[1]->str);
                }
            }
            ins |= (trapvect8 & 0b11111111);
        }

        outputbuf_add(obuf, *p_addr, ins, lidx);
        *p_addr += 1;

    } else if ((pseudo = is_pseudo(toks[0])) != -1) {
        switch (pseudo) {
            case PS_END: { // stop assembling
                end_found = 1;
                break;
            }

            case PS_BLKW: { // reserve `toks[1]` memory locations
                uint8_t type = literal_type(toks[1]);

                if (type != LIT_RES) {
                    uint16_t blank_cnt = parse_literal(type, toks[1]);
                    uint16_t i;

                    for (i = 0; i < blank_cnt; i++) {
                        outputbuf_add(obuf, *p_addr, 0, lidx);
                        *p_addr += 1;
                    }
                } else {
                    error(lidx, "%s is not valid argument", toks[1]->str);
                }
                break;
            }

            case PS_FILL: { // fill the instruction with value defined in toks[1]
                uint8_t type = literal_type(toks[1]);

                if (type != LIT_RES) {
                    uint16_t off = parse_literal(type, toks[1]);
                    outputbuf_add(obuf, *p_addr, off, lidx);
                    *p_addr += 1;
                } else if ((sym_addr = symtab_get(st, toks[1])) > 0) {
                    outputbuf_add(obuf, *p_addr, sym_addr, lidx);
                    *p_addr += 1;
                } else {
                    error(lidx, "%s is not valid argument", toks[1]->str);
                }
                break;
            }

            case PS_STRINGZ: { // place a string starting from current instruction
                uint16_t i;

                for (i = 0; i < toks[1]->size; i++) {
                    outputbuf_add(obuf, *p_addr, toks[1]->str[i], lidx);
                    *p_addr += 1;
                }
                break;
            }

            default: {
                warning(lidx, "%s ignored", toks[0]->str);
            }
        }
    } else {
        error(lidx, "Unknown token %s", toks[0]->str);
    }
    return end_found;
}

void assemble(
    InputBuf *ibuf,     // input buffer (assembly lines)
    OutputBuf *obuf,    // output buffer (machine codes & addresses)
    SymTab *st,         // symbol table (symbol-address pairs)
    FileList *fl,       // list of working files
    char *src_name      // name of assembly source file 
) {
    uint32_t lidx = 1;          // current instruction line index
    uint16_t addr = 0;          // instruction address
    uint32_t orig_idx = 0;      // index in input buffer that .orig is at
    char str[MAX_LINE_LEN + 1]; // store instruction line read from source file
    uint8_t end = 0;            // check if it's the end of each pass 
    Line *ln;
    uint16_t i;

    filelist_open(fl, src_name);
    if (errcnt > 0) {
        return;
    }

    if (find_orig(fl, ibuf, obuf, &lidx)) {
        addr = obuf->instr[0];
        orig_idx = ibuf->size;
    } else {
        error(0, ".ORIG not found");
        return;
    }

    // pass one: read lines from source code file and validate them
    // and create a symbol table
    lidx++;
    for (; fgets(str, MAX_LINE_LEN + 1, fl->src) != NULL; lidx++) {
        ln = line_new(lidx, str);

        // case when line has no instruction
        if (ln->tokcnt == 0) {
            line_del(ln);
            continue;
        }

        end = validate_line(ibuf, st, ln, &addr);
        if (end) {
            break;
        }
    }
    if (errcnt > 0) {
        return;
    }

    // pass two: generate machine codes from assembly instructions and symbol table
    addr = obuf->instr[0]; // move program address back to ORIG

    for (i = orig_idx; i < ibuf->size; i++) {
        ln = inputbuf_get(ibuf, i);
        end = generate_machine_code(obuf, st, ln, &addr);
        if (end) {
            break;
        }
    }
    if (errcnt > 0) {
        return;
    }

    // output to files, write_object() is called last due to the swapping
    // of endianness in the elements of obuf->instr there
    write_sym(st, fl->sym);
    write_lst(obuf, fl->lst, fl->src);
    write_obj(obuf, fl->obj);
}

int main(int argc, char **argv) {
    // show usage guide
    if (argc != 2) {
        printf("%s [asm file]\n", argv[0]);
        exit(1);
    }

    // check if filename argument is an assembly file;
    if (!extension_check(argv[1], ".asm")) {
        printf("%s is not an assembly file\n", argv[1]);
        exit(2);
    }

    OutputBuf *obuf = outputbuf_new();
    InputBuf *ibuf = inputbuf_new();
    SymTab *st = symtab_new();
    FileList *fl = malloc(sizeof *fl);

    assemble(ibuf, obuf, st, fl, argv[1]);

    filelist_close(fl);
    free(fl);
    inputbuf_del(ibuf);
    outputbuf_del(obuf);
    symtab_del(st);

    if (errcnt > 0) {
        clean_output(argv[1]);
    }

    printf("Yields %d error(s) and %d warning(s)\n", errcnt, warncnt);
    return 0;
}
