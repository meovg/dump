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
void error(uint32_t linenum, char *format,...)
{
    va_list args;

    errcnt++;
    va_start(args, format);
    printf("Error: %d: ", linenum);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

// notify a warning on the terminal
void warning(uint32_t linenum, char *format,...)
{
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

char escape_value(char c)
{
    static const char esc_chrs[] = {
        '\'', '\"', '\?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'
    };
    static const char esc_vals[] = {
        0x27, 0x22, 0x3F, 0x5C, 0x07, 0x08, 0x0C, 0x0A, 0x0D, 0x09, 0x0B
    };

    for (int8_t i = 0; i < 11; i++) {
        if (c == esc_chrs[i]) {
            return esc_vals[i];
        }
    }
    return c;
}

int8_t is_eow(char c)
{
    int8_t eow = 0;
    static const char eow_chrs[] = {
        '\0', '\r', '\t', '\n', ' ', ',', ';'
    };

    for (uint8_t i = 0; i < 7; i++) {
        if (c == eow_chrs[i]) {
            eow++;
        }
    }
    return eow;
}

int8_t is_eol(char c)
{
    int8_t eol = 0;
    static const char eol_chrs[] = {
        '\0', '\n', '\r', ';' // semicolon marks the start of a comment
    };

    for (uint8_t i = 0; i < 4; i++) {
        if (c == eol_chrs[i]) {
            eol++;
        }
    }
    return eol;
}

// get a token of substring until the end of a line or a quote
// used for lexing string literals 
Token *token_from_string(const char *str, uint16_t *index_ptr)
{
    uint16_t i = *index_ptr;
    uint16_t j = 0;
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
    *index_ptr = i;

    Token *token = malloc(sizeof *token);

    token->size = j + 1;
    token->str = malloc(token->size * sizeof(char));

    for (uint16_t k = 0; k < j; k++) {
        token->str[k] = tmp[k];
    }
    token->str[j] = '\0';

    return token;
}

// get a token of substring until a word separator is found
// used for lexing symbols, opcodes, psuedo-ops and operands...
Token *token_from_word(const char *str, uint16_t *index_ptr)
{
    uint8_t i = *index_ptr;
    uint8_t j = 0;
    static char tmp[MAX_TOKEN_LEN + 1];

    while (!is_eow(str[i])) {
        tmp[j] = str[i];
        j++;
        i++;
    }

    // index should be at the last character in a token
    // so that tokenize() can check if the next character is a separator
    *index_ptr = i - 1;

    Token *token = malloc(sizeof *token);

    token->size = j + 1;
    token->str = malloc(token->size * sizeof(char));

    for (uint8_t k = 0; k < j; k++) {
        token->str[k] = tmp[k];
    }
    token->str[j] = '\0';

    return token;
}

typedef struct {
    Token **tokens;         // array of tokens in an instruction line
    uint8_t token_count;    // token count of the line
    uint32_t line_num;      // the line number (index) in the assembly file
} Line;

// break down an instruction line into several chunks of strings (tokens)
Line *tokenize(uint32_t linenum, const char *str)
{
    Line *line = malloc(sizeof *line);

    line->line_num = linenum;
    line->token_count = 0;
    line->tokens = malloc(sizeof(Token *));

    int8_t need_token = 0;  // check if it's necessary to reserve space in
                            // token array, set to 1 if token array is full
    int8_t is_sep = 0;
    int8_t is_quote = 0;
    int8_t prev_is_sep = 1; // check if the previous character is a separator to
                            // extract token from a word, thus set to 1 at first

    for (uint16_t i = 0; !is_eol(str[i]); i++) {
        is_sep = (str[i] == ' ' || str[i] == '\t' || str[i] == ',');

        if (str[i] == '\"') {
            i++;
            line->tokens[line->token_count] = token_from_string(str, &i);
            line->token_count++;
            need_token = 1;
        } else if (prev_is_sep && !is_sep) {
            line->tokens[line->token_count] = token_from_word(str, &i);
            line->token_count++;
            is_sep = 1;
            need_token = 1;
        }

        // not necessary to realloc 2 times the current capacity
        // as the limit is small (7 tokens per line)
        if (need_token) {
            line->tokens = realloc(line->tokens,
                    (line->token_count + 1) * sizeof(Token *));
            need_token = 0;
        }

        prev_is_sep = is_sep;
    }
    return line;
}

// deallocate the token
void free_token(Token *token)
{
    if (token) {
        if (token->str) {
            free(token->str);
        }
        free(token);
    }
}

// deallocate a Line object and the tokens contained inside
void free_line(Line *line)
{
    if (!line) {
        return;
    }
    if (line->tokens) {
        for (uint8_t i = 0; i < line->token_count; i++) {
            if (line->tokens[i]->size) {
                free_token(line->tokens[i]);
            }
        }
        free(line->tokens);
    }
    free(line);
}

// dynamic container of Line objects
typedef struct {
    Line **arr;
    uint32_t size;
    uint32_t cap;
} LineList;

#define LINELIST_DEFAULT_SIZE 200

LineList *create_line_list(void)
{
    LineList *lines = malloc(sizeof *lines);

    lines->cap = LINELIST_DEFAULT_SIZE;
    lines->size = 0;
    lines->arr = malloc(lines->cap * sizeof(Line *));

    return lines;
}

void add_line(LineList *lines, Line *line)
{
    if (lines->size == lines->cap) {
        lines->cap *= 2;
        lines->arr = realloc(lines->arr, lines->cap * sizeof(Line *));
    }

    lines->arr[lines->size] = line;
    lines->size++;
}

// retrieve a Line object based on the index
Line *get_line(LineList *lines, uint32_t idx)
{
    if (idx >= lines->size) {
        return NULL;
    }
    return lines->arr[idx];
}

// deallocate Line list and the lines contained
void free_line_list(LineList *lines)
{
    if (!lines) {
        return;
    }

    for (uint32_t i = 0; i < lines->size; i++) {
        free_line(lines->arr[i]);
    }

    free(lines->arr);
    free(lines);
}

/* //////// output buffer //////// */

// storing assembled machine codes and related infos to write into
// program's object file and listing file
typedef struct {
    uint16_t *instructs;    // array of machine codes
    uint16_t *addresses;    // array of addresses of machine codes
    uint32_t *line_nums;    // array of indexes of instructions in the assembly
                            // file that map to respective machine codes
    uint16_t size;
    uint16_t cap;
} OutputBuffer;

#define OUTPUT_BUFFER_DEFAULT_SIZE 128

OutputBuffer *create_output_buffer(void)
{
    OutputBuffer *outbuf = malloc(sizeof *outbuf);

    outbuf->size = 0;
    outbuf->cap = OUTPUT_BUFFER_DEFAULT_SIZE;

    outbuf->addresses = malloc(outbuf->cap * sizeof(uint16_t));
    outbuf->instructs = malloc(outbuf->cap * sizeof(uint16_t));
    outbuf->line_nums = malloc(outbuf->cap * sizeof(uint32_t));

    return outbuf;
}

// add assembled instructions, address and line_num to output buffer
// to be used later on
void add_to_output_buffer(
    OutputBuffer *outbuf,
    uint16_t address,
    uint16_t instruction,
    uint32_t line_num)
{
    if (outbuf->size == outbuf->cap) {
        outbuf->cap *= 2;
        outbuf->addresses = realloc(outbuf->addresses,
                outbuf->cap * sizeof(uint16_t));
        outbuf->instructs = realloc(outbuf->instructs,
                outbuf->cap * sizeof(uint16_t));
        outbuf->line_nums = realloc(outbuf->line_nums,
                outbuf->cap * sizeof(uint32_t));
    }

    outbuf->addresses[outbuf->size] = address;
    outbuf->instructs[outbuf->size] = instruction;
    outbuf->line_nums[outbuf->size] = line_num;

    outbuf->size++;
}

void free_output_buffer(OutputBuffer *outbuf)
{
    if (!outbuf) {
        return;
    }
    if (outbuf->addresses) {
        free(outbuf->addresses);
    }
    if (outbuf->instructs) {
        free(outbuf->instructs);
    }
    if (outbuf->line_nums) {
        free(outbuf->line_nums);
    }
    free(outbuf);
}

/* //////// symbol table and helpers //////// */

typedef struct {
    Token *key;         // symbol (label)
    uint16_t address;   // symbol's instruction address
} Symbol;

typedef struct {
    Symbol **arr;
    uint32_t size;
    uint32_t cap;
} SymbolTable;

#define SYMBOL_TABLE_DEFAULT_SIZE 8

void add_symbol(SymbolTable *symtab, Token *label, uint16_t address)
{
    if (symtab->size == symtab->cap) {
        symtab->cap *= 2;
        symtab->arr = realloc(symtab->arr, symtab->cap * sizeof(Symbol *));
    }

    symtab->arr[symtab->size] = malloc(sizeof(Symbol));
    Symbol *sym = symtab->arr[symtab->size];

    // allocate and copy the contents from label token to sym->key
    sym->key = malloc(sizeof(Token));
    sym->key->size = label->size;

    sym->key->str = malloc(label->size * sizeof(char));
    memcpy(sym->key->str, label->str, label->size);

    sym->address = address;

    symtab->size++;
}

SymbolTable *create_symbol_table(void)
{
    SymbolTable *symtab = malloc(sizeof(*symtab));

    symtab->size = 0;
    symtab->cap = SYMBOL_TABLE_DEFAULT_SIZE;
    symtab->arr = malloc(symtab->cap * sizeof(Symbol *));

    return symtab;
}

void free_symbol_table(SymbolTable *symtab)
{
    if (!symtab) {
        return;
    }

    for (uint16_t i = 0; i < symtab->size; i++) {
        free_token(symtab->arr[i]->key);
        free(symtab->arr[i]);
    }

    free(symtab->arr);
    free(symtab);
}

// get the instruction address of a symbol from the symbol table
uint16_t symbol_address(SymbolTable *symtab, Token *key)
{
    for (uint16_t i = 0; i < symtab->size; i++) {
        if (!strcmp(symtab->arr[i]->key->str, key->str)) {
            return symtab->arr[i]->address;
        }
    }
    return 0;
}

// check if an instruction address has a symbol referring to it
// or return NULL
Token *exist_symbol(SymbolTable *symtab, uint16_t address)
{
    for (uint16_t i = 0; i < symtab->size; i++) {
        if (symtab->arr[i]->address == address) {
            return symtab->arr[i]->key;
        }
    }
    return NULL;
}

/* //////// Files output //////// */

// group of files to work with
typedef struct {
    FILE *src;  // assembly file (read plain text)
    FILE *sym;  // symbol table file (write plain text)
    FILE *obj;  // machine code file (write binary)
    FILE *lst;  // listing file (write plain text)
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

// open files to work with
void open_file_list(FileList *fl, char *filename)
{
    uint8_t err = 0;

    fl->src = fopen(filename, "r");
    if (fl->src == NULL) {
        error(0, "Couldn't open assembly file: %s", filename);
    }

    replace_extension(filename, ".sym");
    fl->sym = fopen(filename, "w");
    if (fl->sym == NULL) {
        error(0, "Couldn't create symbol table file: %s", filename);
    }

    replace_extension(filename, ".obj");
    fl->obj = fopen(filename, "wb");  // binary file
    if (fl->obj == NULL) {
        error(0, "Couldn't create object file: %s", filename);
    }

    replace_extension(filename, ".lst");
    fl->lst = fopen(filename, "w");
    if (fl->lst == NULL) {
        error(0, "Couldn't create listing file: %s", filename);
    }

    replace_extension(filename, ".asm");
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

// delete unused output files
// only used when errors occur during assembling process
void clean_output(char *filename)
{
    static const char *exts[] = {".sym", ".obj", ".lst"};

    printf("\nCleaning up output files...\n");

    for (uint8_t i = 0; i < 3; i++) {
        replace_extension(filename, exts[i]);
        if (remove(filename)) {
            warning(0, "Couldn't delete %s", filename);
        }
    }

    replace_extension(filename, ".asm");
}

// write assembled machine instructions to .obj file
void write_object(OutputBuffer *outbuf, FILE *obj)
{
    // (the struggle to swap the byte order of machine instructions
    // on both the assembler and simulator just because LC-3 uses big endian)
    // i guess object file is written last then...
    for (uint16_t i = 0; i < outbuf->size; i++) {
        uint16_t tmp = outbuf->instructs[i];
        outbuf->instructs[i] = (tmp << 8) | (tmp >> 8);
    }

    fwrite(outbuf->instructs, sizeof(uint16_t), outbuf->size, obj);
}

// write the symbol table to .sym file
// symbols are listed in the second column while their addresses are in the first
void write_symbol_table(SymbolTable *symtab, FILE *sym)
{
    fprintf(sym, "  Addr  |  Symbol\n");

    for (uint32_t idx = 0; idx < symtab->size; idx++) {
        fprintf(sym, " x%04X  |  %s\n",
                symtab->arr[idx]->address, symtab->arr[idx]->key->str);
    }
}

static const char *binseq[16] = {
    "0000", "0001", "0010", "0011",
    "0100", "0101", "0110", "0111",
    "1000", "1001", "1010", "1011",
    "1100", "1101", "1110", "1111"
};

// write listing table to .lst file
// each column of table specifies assembled machine code addresses in hex (1)
// instructions in hex (2) and binary (3), the index (4) and content (5)
// of respective source codes in assembly file
//
// note that a line in source code can map to 0, 1 or many machine instructions
void write_listing(OutputBuffer *outbuf, FILE *lst, FILE *src)
{
    // sequence used to print instructions in binary
    #define PRbinseq(x) \
        binseq[x >> 12], \
        binseq[(x >> 8) & 0xf], \
        binseq[(x >> 4) & 0xf], \
        binseq[x & 0xf]

    static char line[MAX_LINE_LEN + 1];

    uint16_t idx = 1;
    uint16_t lim = outbuf->size;

    fseek(src, 0, SEEK_SET);

    fprintf(lst, "  Addr  |  Hex  |       Bin        | Line |  Source\n");

    for (uint32_t ln = 1; fgets(line, MAX_LINE_LEN + 1, src); ln++) {
        // write first 3 columns if any matched instruction is found
        // or leave them blank
        if (idx != lim && outbuf->line_nums[idx] == ln) {
            uint16_t instruction = outbuf->instructs[idx];
            uint16_t address = outbuf->addresses[idx];

            fprintf(lst, " x%04X  | x%04X | %s%s%s%s ",
                    address, instruction, PRbinseq(instruction));

            idx++;
        } else {
            fprintf(lst, "        |       |                  ");
        }

        // write respective line in source code
        // fgets reads newline to `line` as well so there's no need to add \n
        fprintf(lst, "| %4d | %s", ln, line);

        // write other machine codes in case one assembly code maps to
        // multiple machine codes (.STRINGZ and .BLKW)
        while (idx != lim && outbuf->line_nums[idx] == ln) {
            uint16_t instruction = outbuf->instructs[idx];

            fprintf(lst, "        | x%04X | %s%s%s%s |      |\n",
                    instruction, PRbinseq(instruction));
            idx++;
        }
    }
}

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
int8_t is_register(Token *token)
{
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

    for (int8_t i = 0; i < 16; i++) {
        if (!strcmp(token->str, registers[i])) {
            ret = i / 2;
            break;
        }
    }

    return ret;
}

// check if token is correct branch options or return the default BRnzp
int8_t is_branch(Token *token)
{
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

    for (int8_t i = 0; i < 24; i++) {
        if (!strcmp(token->str, br_options[i])) {
            ret = i / 3;
            break;
        }
    }

    ret = (ret == 0) ? 7 : ret;
    return ret;
}

// check if token is trapcode or return -1
int8_t is_trap(Token *token)
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
        if (!strcmp(token->str, traps[i])) {
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
int8_t is_opcode(Token *token)
{
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

    for (int8_t i = 0; i < 32; i++) {
        if (!strcmp(token->str, opcodes[i])) {
            ret = i / 2;
            break;
        }
    }

    if (ret < 0) { // check if token is trapcode or opcode not defined above
        if (is_branch(token) >= 0) {
            ret = OP_BR;
        } else if (is_trap(token) >= 0) {
            ret = OP_TRAPS;
        } else if (!strcmp(token->str, "JSRR") || !strcmp(token->str, "jsrr")) {
            ret = OP_JSRR;
        } else if (!strcmp(token->str, "RET") || !strcmp(token->str, "ret")) {
            ret = OP_RET;
        }
    } else if (ret == OP_RES) {
        ret = -1;
    }

    return ret;
}

int8_t is_pseudo(Token *token)
{
    static const char *pseudos[] = {
        ".ORIG", ".orig",
        ".END", ".end",
        ".BLKW", ".blkw",
        ".FILL", ".fill",
        ".STRINGZ", ".stringz"
    };

    int8_t ret = -1;

    for (int8_t i = 0; i < 10; i++) {
        if (!strcmp(token->str, pseudos[i])) {
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
uint8_t is_valid_symbol(Token *token)
{
    char *s = token->str;

    if (!isalpha(s[0]) && s[0] != '_') {
        return 0;
    }

    for (uint16_t i = 0; s[i] != '\0'; i++) {
        if (!isalnum(s[i]) && s[i] != '_') {
            return 0;
        }
    }

    // no need to check if it's a pseudo-op as the first character cannot be period
    return is_register(token) == -1 && is_opcode(token) == -1;
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
int16_t parse_two_complement(const char *s)
{
    static const int N = 16;
    static char seq[17];

    memset(seq, 0, 17 * sizeof(char));

    int16_t res = 0;
    size_t len = strlen(s);
    size_t i = len - 1, j = 0;

    while (j < len && j < N) {
        seq[j] = s[i] - '0';
        j++;
        i--;
    }

    for (i = 0; i <= N-2; i++) {
        res += seq[i] * (1 << i);
    }

    res -= seq[N-1] * (1 << (N-1));

    return res;
}

// get decimal value of hex digit
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

// calculate the two's complement literal represented in hex code
int16_t parse_hex(const char *s)
{
    static const int N = 4;
    static char seq[5]; // storing the 4 bits to convert since i know some of you
                        // will go all your way to drop a 1000-digit number here
    static char bin[17]; // binary sequence to be used in parse_two_complement()

    memset(seq, 0, 5 * sizeof(char));
    memset(bin, 0, 17 * sizeof(char));

    // move to the start of actual number in literal
    char *num = strchr(s, 'x');
    if (num == NULL) {
        num = strchr(s, 'X'); // reminder to stop abusing the left shift key
                              // right shift key's life matters
    }

    // skip that x too
    num++;

    // copy the thing to seq, i think
    size_t len = strlen(num);
    size_t i = len - 1, j = 0;

    while (j < len && j < N) {
        seq[j] = (char)hex_index(num[i]);
        j++;
        i--;
    }

    // transcode into binary sequence
    i = 0;
    char *tmp = bin + 16;

    while (i < N) {
        tmp -= 4 * sizeof(char);
        memcpy(tmp, binseq[seq[i]], 4 * sizeof(char));
        i++;
    }

    return parse_two_complement(bin);
}

int16_t parse_bin(char *s)
{
    char *num = strchr(s, 'b');
    if (num == NULL) {
        num = strchr(s, 'B'); // right key also deserves care and support
    }

    num++;

    return parse_two_complement(num);
}

int16_t parse_dec(char *s)
{
    int8_t neg = 0;
    char *num = strchr(s, '#');

    if (num == NULL) {
        num = s;
    } else {
        num++;
    }

    if (num[0] == '-') {
        neg = 1;
        num++;
    }

    int16_t res = 0;
    size_t i = 0;

    while (num[i] != '\0') {
        res = res * 10 + (num[i] - '0');
        i++;
    }

    return neg ? -res : res;
}

int16_t parse_literal(uint8_t literal_type, Token *token)
{
    char *s = token->str;
    int16_t val = 0;

    if (literal_type == LIT_HEX) {
        val = parse_hex(s);
    } else if (literal_type == LIT_BIN) {
        val = parse_bin(s);
    } else if (literal_type == LIT_DEC) {
        val = parse_dec(s);
    }
    return val;
}

// return the literal type of the presumably "literal" token
uint8_t literal_type(Token *token)
{
    char *s = token->str;
    char *literal;

    // hexadecimal type: x$$$$ or X$$$$ format (no negative sign)
    // $ in range [0..9, A..F]
    if ((literal = strchr(s, 'x')) || (literal = strchr(s, 'X'))) {
        literal++;

        for (; *literal; literal++) {
            if (hex_index(*literal) == -1) {
                return LIT_RES;
            }
        }
        return LIT_HEX;

    // binary type: b$$...$ or B$$...$ format (no negative sign)
    // $ is either 0 or 1
    } else if ((literal = strchr(s, 'b')) || (literal = strchr(s, 'B'))) {
        literal++;

        for (; *literal; literal++) {
            if (*literal != '0' && *literal != '1') {
                return LIT_RES;
            }
        }
        return LIT_BIN;

    // decimal type: #$$$..$ or $$$..$ format (1 negative sign allowed)
    } else if (s != NULL) {
        literal = strchr(s, '#');

        if (literal == NULL) {
            literal = s;
        } else {
            literal++;
        }

        if (*literal == '-') {
            literal++;
        }

        for (; *literal; literal++) {
            if (!isdigit(*literal)) {
                return LIT_RES;
            }
        }
        return LIT_DEC;

    } else {
        return LIT_RES;
    }
}

/* //////// actual assembling routines //////// */

// find pseudo-op .ORIG and get the address to the start of the code
uint16_t find_orig(
    FileList *files,
    OutputBuffer *outbuf,
    LineList *lines,
    uint32_t *linenum)
{
    static char str[MAX_LINE_LEN + 1]; // char array to read lines from files
    uint16_t orig = 0;
    uint32_t i;

    for (i = *linenum; fgets(str, MAX_LINE_LEN + 1, files->src) != NULL; i++) {
        Line *line = tokenize(i, str);
        if (line->token_count == 0) {
            free_line(line);
            continue;
        }

        int8_t pseudo_idx = is_pseudo(line->tokens[0]);

        // found .ORIG
        if (pseudo_idx == PS_ORIG) {
            orig = parse_literal(LIT_HEX, line->tokens[1]);
            add_to_output_buffer(outbuf, 0, orig, i);

            add_line(lines, line);
            break;
        } else {
            warning(i, "%s instruction ignored", line->tokens[0]);
        }
    }

    *linenum = i;

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
    LineList *lines,
    SymbolTable *symtab,
    Line *line,
    uint16_t *addr)
{
    uint8_t end_found = 0;      // return value, set to 1 if .END is found
    uint32_t ln = line->line_num;
    char *tmp;
    int8_t opcode = -1;         // opcode index defined in some enum idk
    int8_t pseudo = -1;         // same but for pseudo-ops
    int8_t operand_exp = -1;    // expected operand counts
    int8_t operand_cnt = 0;     // actual operand counts
    Token *token;

    for (uint8_t i = 0; i < line->token_count; i++) {
        token = line->tokens[i];

        if (i == 0 && is_valid_symbol(token)) {
            // check if any symbol also points to the current address
            Token *dupe = exist_symbol(symtab, *addr);
            if (dupe == NULL) {
                add_symbol(symtab, token, *addr);
            } else {
                warning(ln, "Another symbol is pointing to address %04x: %s",
                        *addr, dupe->str);
            }

            operand_exp = 0;
            tmp = token->str;
        } else if ((opcode = is_opcode(token)) != -1) {
            operand_exp = opcode_operand_cnt[opcode];
            tmp = token->str;
            *addr += 1;
        } else if ((pseudo = is_pseudo(token)) != -1) {
            operand_exp = pseudo_operand_cnt[pseudo];
            tmp = token->str;

            if (pseudo == PS_END) { // .END is found, leave now
                end_found = 1;
                break;
            }

            // this is already an "operand not found" error but
            // let's propagate that to operand check at the end of the function
            if (i + 1 >= line->token_count) {
                break;
            }

            Token *next = line->tokens[i + 1];

            if (pseudo == PS_STRINGZ) {
                // offset is the length of string literal
                *addr += next->size;
                i++;
                operand_cnt++;
            } else if (pseudo == PS_BLKW) {
                // offset the literal
                uint8_t type = literal_type(next);
                if (type != LIT_RES) {
                    *addr += parse_literal(type, next);
                }
            } else if (pseudo == PS_FILL) {
                *addr += 1;
            } else {
                warning(ln, "%s ignored", token->str);
            }
        } else if (is_register(token) != -1         // valid operand check
                || literal_type(token) != LIT_RES
                || is_valid_symbol(token)) {
            operand_cnt++;
        } else {
            error(ln, "Unknown token %s", token->str);
        }
    }

    if (operand_exp == -1) {

    } else if (operand_cnt > operand_exp) {
        warning(ln, "%s has more operands than expected", line->tokens[0]->str);
    } else if (operand_cnt < operand_exp) {
        error(ln, "%s has not enough operands", line->tokens[0]->str);
    }

    // add current line to the line list so the program can move on from
    // reading this forsaken source code file
    add_line(lines, line);

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
    OutputBuffer *outbuf,
    SymbolTable *symtab,
    Line *line,
    uint16_t *address)
{
    uint8_t end_found = 0;
    uint32_t ln = line->line_num;
    Token **tokens = line->tokens;
    int8_t opcode = -1;
    int8_t pseudo = -1;

    uint16_t sym_addr = symbol_address(symtab, tokens[0]);

    if (sym_addr) {
        if (line->token_count == 1) {
            return 0;
        }
        tokens++;
    }

    if ((opcode = is_opcode(tokens[0])) != -1) {
        uint16_t ins = opcode_val[opcode] << 12;
        uint8_t opmask = operand_mask[opcode];

        // NOT opcode gets special treatment as its instruction has 6 lowest bits
        // filled with 1, aside from Dst and Src1
        if (opcode == OP_NOT) {
            ins |= 0b111111;
        }

        if (opmask & DstSrc) {
            int8_t reg = is_register(tokens[1]);
            if (reg >= 0) {
                ins |= (reg << 9);
            } else {
                error(ln, "%s is not a register", tokens[1]->str);
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

            if (reg == 7 || (reg = is_register(tokens[rhs])) != -1) {
                ins |= (reg << 6);
            } else {
                error(ln, "%s is not a register", tokens[rhs]->str);
            }
        }

        // also handles Imm5 if token is not a register
        if (opmask & Src2) {
            int8_t reg = 0;
            uint8_t type = 0;

            if ((reg = is_register(tokens[3])) != -1) {
                ins |= reg;
            } else if ((type = literal_type(tokens[3])) != LIT_RES) {
                int16_t imm5 = parse_literal(type, tokens[3]);
                if (!in_range_5_bit(imm5)) {
                    error(ln, "Immediate %s cannot be represented in 5 bits",
                            tokens[3]->str);
                }
                ins |= (imm5 & 0b11111);

                // the 1 in the 5th indicates immediate mode
                // telling machine to inteprete the lowest 5 bits as literal
                ins |= (1 << 5);
            } else {
                error(ln, "%s is not a valid operand for Src2", tokens[3]->str);
            }
        }

        // no need to check since the default branch option is BRnzp
        if (opmask & Cond) {
            ins |= (is_branch(tokens[0]) << 9);
        }

        if (opmask & PCoff9) {
            uint8_t type = 0;
            uint8_t rhs = opcode == OP_BR ? 1 : 2;
            int16_t off = 0;

            sym_addr = symbol_address(symtab, tokens[rhs]);
            if (sym_addr) {
                off = sym_addr - (*address + 1);

                if (!in_range_9_bit(off)) {
                    error(ln, "PCoffset %04x cannot be represented in 9 bits",
                            off);
                }

                ins |= (off & 0b111111111);
            } else if ((type = literal_type(tokens[rhs])) != LIT_RES) {
                off = parse_literal(type, tokens[rhs]);

                if (!in_range_9_bit(off)) {
                    error(ln, "PCoffset %04x cannot be represented in 9 bits",
                            off);
                }
                ins |= (off & 0b111111111);
            } else {
                error(ln, "%s is not a valid operand for PCoff9",
                        tokens[rhs]->str);
            }
        }

        if (opmask & PCoff11) { // exclusive to JSR
            uint8_t type = 0;
            int16_t off = 0;

            ins |= (1 << 11);

            sym_addr = symbol_address(symtab, tokens[1]);

            if (sym_addr) {
                off = sym_addr - (*address + 1);

                if (!in_range_11_bit(off)) {
                    error(ln, "PCoffset %04x cannot be represented in 11 bits",
                            off);
                }

                ins |= (off & 0b11111111111);
            } else if ((type = literal_type(tokens[1])) != LIT_RES) {
                off = parse_literal(type, tokens[1]);

                if (!in_range_11_bit(off)) {
                    error(ln, "PCoffset %04x cannot be represented in 11 bits",
                            off);
                }
                ins |= (off & 0b11111111111);
            } else {
                error(ln, "%s is not a valid operand for PCoff11",
                        tokens[1]->str);
            }
        }

        if (opmask & Off6) {
            uint8_t type = literal_type(tokens[3]);
            int16_t off = 0;

            if (type != LIT_RES) {
                off = parse_literal(type, tokens[3]);

                if (!in_range_6_bit(off)) {
                    error(ln, "Offset %04x cannot be represented in 6 bits",
                            off);
                }
                ins |= (off & 0b111111);
            } else {
                error(ln, "%s is not a valid operand for Off6",
                        tokens[3]->str);
            }
        }

        // trapcode, validation is at validate_line() (inside is_opcode)
        if (opmask & Tvec8) {
            int16_t trapvect8 = is_trap(tokens[0]);

            // case instruction is in `TRAP <trapvect8>` format
            if (trapvect8 == 0) {
                uint8_t type = literal_type(tokens[1]);

                if (type != LIT_RES) {
                    trapvect8 = parse_literal(type, tokens[1]);

                    if (trapvect8 < 0 || trapvect8 > 0xff) {
                        error(ln, "%s is not a valid trapvector",
                                tokens[1]->str);
                    } else if (trapvect8 < 0x20 || trapvect8 > 0x25) {
                        warning(ln, "Undefined trap routine %s ignored",
                                tokens[1]->str);
                    }
                } else {
                    error(ln, "%s is not a valid trapvector", tokens[1]->str);
                }
            }
            ins |= (trapvect8 & 0b11111111);
        }

        add_to_output_buffer(outbuf, *address, ins, ln);
        *address += 1;

    } else if ((pseudo = is_pseudo(tokens[0])) != -1) {
        switch (pseudo) {
        case PS_END: { // stop assembling
            end_found = 1;
            break;
        }

        case PS_BLKW: { // reserve `tokens[1]` memory locations
            uint8_t type = literal_type(tokens[1]);

            if (type != LIT_RES) {
                uint16_t blank_cnt = parse_literal(type, tokens[1]);

                for (uint16_t i = 0; i < blank_cnt; i++) {
                    add_to_output_buffer(outbuf, *address, 0, ln);
                    *address += 1;
                }
            } else {
                error(ln, "%s is not valid argument", tokens[1]->str);
            }
            break;
        }

        case PS_FILL: { // fill the instruction with value defined in tokens[1]
            uint8_t type = literal_type(tokens[1]);

            if (type != LIT_RES) {
                uint16_t off = parse_literal(type, tokens[1]);
                add_to_output_buffer(outbuf, *address, off, ln);
                *address += 1;
            } else if ((sym_addr = symbol_address(symtab, tokens[1])) > 0) {
                add_to_output_buffer(outbuf, *address, sym_addr, ln);
                *address += 1;
            } else {
                error(ln, "%s is not valid argument", tokens[1]->str);
            }
            break;
        }

        case PS_STRINGZ: { // place a string starting from current instruction
            for (uint16_t i = 0; i < tokens[1]->size; i++) {
                add_to_output_buffer(outbuf, *address, tokens[1]->str[i], ln);
                *address += 1;
            }
            break;
        }

        default: {
            warning(ln, "%s ignored", tokens[0]->str);
        }
        }
    } else {
        error(ln, "Unknown token %s", tokens[0]->str);
    }

    return end_found;
}

void assemble(
    OutputBuffer *outbuf,
    LineList *lines,
    SymbolTable *symtab,
    FileList *files,
    char *filename)
{
    open_file_list(files, filename);

    if (errcnt > 0) {
        return;
    }

    uint32_t line_num = 1;
    uint16_t address = 0;
    uint32_t orig_idx = 0;

    if (find_orig(files, outbuf, lines, &line_num)) {
        address = outbuf->instructs[0];
        orig_idx = lines->size;
    } else {
        error(0, ".ORIG not found");
        return;
    }

    char str[MAX_LINE_LEN + 1];
    uint8_t end = 0;

    // pass one: read lines from source code file and validate them
    // and create a symbol table
    line_num++;
    for (; fgets(str, MAX_LINE_LEN + 1, files->src) != NULL; line_num++) {
        Line *line = tokenize(line_num, str);

        // case when line has no instruction
        if (line->token_count == 0) {
            free_line(line);
            continue;
        }

        end = validate_line(lines, symtab, line, &address);
        if (end) {
            break;
        }
    }

    if (errcnt > 0) {
        return;
    }

    // pass two: generate machine codes from assembly instructions and symbol table
    uint16_t idx;
    address = outbuf->instructs[0]; // move program address back to ORIG

    for (idx = orig_idx; idx < lines->size; idx++) {
        Line *line = get_line(lines, idx);
        generate_machine_code(outbuf, symtab, line, &address);
    }

    if (errcnt > 0) {
        return;
    }

    // output to files, write_object() is called last due to the swapping
    // of endianness in the elements of outbuf->instructs there
    write_symbol_table(symtab, files->sym);
    write_listing(outbuf, files->lst, files->src);
    write_object(outbuf, files->obj);
}

int main(int argc, char **argv)
{
    // show usage guide
    if (argc != 2) {
        printf("%s [asm file]\n", argv[0]);
        exit(1);
    }

    // check if filename argument is an assembly file;
    if (!check_extension(argv[1], ".asm")) {
        printf("%s is not an assembly file", argv[1]);
        exit(2);
    }

    OutputBuffer *outbuf = create_output_buffer();
    LineList *lines = create_line_list();
    SymbolTable *symtab = create_symbol_table();
    FileList *files = malloc(sizeof *files);

    assemble(outbuf, lines, symtab, files, argv[1]);

    close_file_list(files);
    free(files);
    free_line_list(lines);
    free_output_buffer(outbuf);
    free_symbol_table(symtab);

    if (errcnt > 0) {
        clean_output(argv[1]);
    }

    printf("Yields %d error(s) and %d warning(s)\n", errcnt, warncnt);
    return 0;
}
