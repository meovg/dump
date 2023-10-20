#ifndef COMMON_H_
#define COMMON_H_

#include <inttypes.h>
#include <stdio.h>

// opcodes used in LC-3 instructions
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

// pseudo-ops used in LC-3 instructions
enum PseudoOpcode {
    PS_ORIG,    // notify where program code begins
    PS_END,     // tell assembler to stop assemble
    PS_BLKW,    // reserve memory locations
    PS_FILL,    // place value at that line of code
    PS_STRINGZ, // place a string literal
};

// ********** defined in utils.c **********

// array of 4-bit binary sequence for each hex digit 
extern char *binseq[16];

// gets the escape value from bare escape character without prefix '\'
// i.e. escape_value('n') returns the newline
char escape_value(char c);

// checks if character is end-of-word marker
int8_t is_eow(char c);
// checks if character is end-of-line marker
int8_t is_eol(char c);

// wrappers to abort when OOM occurs on alloc and realloc
void *mymalloc(size_t size);
void *myrealloc(void *ptr, size_t size);

// informs error to the standard output
void set_error(uint32_t lidx, char *format, ...);
// informs warning to the standard output
void set_warning(uint32_t lidx, char *format, ...);
// returns error count
uint32_t get_error_count(void);
// returns warning count
uint32_t get_warning_count(void);

// ********** defined in token.c *********

#define MAX_TOKEN_NUM 7
#define MAX_TOKEN_LEN 1025
#define MAX_LINE_LEN 7175   // MAX_TOKEN_NUM * MAX_TOKEN_LEN

typedef enum {
    TK_STR,
    TK_OPCODE,
    TK_PSEUDO,
    TK_REGISTER,
    TK_LITERAL,
    TK_SYMBOL,
    TK_INVALID
} TokenType;

// string block in an instruction line
typedef struct {
    char *str;
    TokenType type;
    int8_t subtype;
    uint16_t size;
    uint32_t lidx;
} Token;

// creates a token from a string at specific index until an end-of-line character is found
Token *token_get_until_eol(uint32_t lidx, const char *str, uint16_t *p_idx);
// creates a token from a string at specific index until an end-of-word character is found
Token *token_get_until_eow(uint32_t lidx, const char *str, uint16_t *p_idx);
// deallocates the token and its content
void token_del(Token *tok);
// returns a copy of token object
Token *token_copy(Token *src);

// ********** defined in line.c **********

// line info of a lexed instructions
typedef struct {
    Token **tokens;     // array of tokens in an instruction line
    uint8_t tokcnt;     // token count of the line
} Line;

// creates line object from instruction line
Line *line_new(uint32_t lidx, const char *str);
// deallocates the line object and its content
void line_del(Line *ln);

// ********** defined in buffer.c **********

#define INPUTBUF_DEFAULT_SIZE 200

// container of Line objects
typedef struct {
    Line **arr;
    uint32_t size;
    uint32_t cap;
} InputBuf;

// creates empty input buffer
InputBuf *inputbuf_new(void);
// adds line object into input buffer
void inputbuf_add(InputBuf *ibuf, Line *ln);
// retrieves line object from input buffer using index
Line *inputbuf_get(InputBuf *ibuf, uint32_t idx);
// deallocates input buffer and its content
void inputbuf_del(InputBuf *ibuf);

#define OUTPUTBUF_DEFAULT_SIZE 128

// contains containers of instructions, addresses, line indices for output
typedef struct {
    uint16_t *instr;    // machine codes
    uint16_t *addr;     // addresses of machine codes
    uint32_t *lidx;     // indices of instructions in the assembly
                        // file that map to respective machine codes
    uint16_t size;
    uint16_t cap;
} OutputBuf;

// creates empty output buffer
OutputBuf *outputbuf_new(void);
// adds machine instruction, address and line index into output buffer
void outputbuf_add(OutputBuf *obuf, uint16_t addr, uint16_t instr, uint32_t lidx);
// deallocates output buffer and its content
void outputbuf_del(OutputBuf *obuf);

// ********** defined in symbol.c **********

// the symbol table is a chained hash table containing an array of
// linked lists (or buckets) of Symbol objects

#define SYMTAB_DEFAULT_BKTCNT 8

// an entry in symbol table
typedef struct symentry {
    Token *key;             // instruction label
    uint16_t addr;          // instruction address
    uint32_t hash;          // hash value of this entry, from the hash function
    struct symentry *next;  // next object in bucket, NULL if the object is the tail 
} Symbol;

typedef struct {
    Symbol **buckets;       // array of Symbol object linked lists
    uint32_t size;          // number of Symbol objects in the table
    uint32_t bktcnt;        // bucket count of the table
} SymTab;

// creates empty symbol table
SymTab *symtab_new(void);
// adds to symbol table
void symtab_add(SymTab *st, Token *key, uint16_t addr);
// deallocates symbol table
void symtab_del(SymTab *st);
// finds symbol entry
Symbol *symtab_find(SymTab *st, Token *token);
// returns symbol entry mapping to the address
Symbol *symtab_find_by_value(SymTab *st, uint16_t addr);

// ********** defined in file.c *********

// group of files to work on
typedef struct {
    FILE *src;  // assembly file (read plain text)
    FILE *sym;  // symbol table file (write plain text)
    FILE *obj;  // machine code file (write binary, big-endian)
    FILE *bin;  // machine code file (write plain text, little-endian)
    FILE *lst;  // listing file (write plain text)
} FileList;

// extension helpers
int8_t extension_check(char *filename, const char *ext);
void extension_replace(char *filename, const char *ext);

// opens source code file and files to output
void filelist_open(FileList *fl, char *filename);
// closes all working files
void filelist_close(FileList *fl);

// clears all output files (used when errors occur)
void clean_output(char *filename);

// write output to multiple files
void write_obj(OutputBuf *obuf, FILE *obj);
void write_sym(SymTab *st, FILE *sym);
void write_lst(OutputBuf *obuf, FILE *lst, FILE *src);
void write_bin(OutputBuf *obuf, FILE *bin);

// ********** defined in validate.c **********

// returned value of -1 indicates invalid token
int8_t get_register_type(Token *tok);
int8_t get_branch_type(Token *tok);
int8_t get_trap_type(Token *tok);
int8_t get_opcode_type(Token *tok);
int8_t get_pseudo_type(Token *tok);
uint8_t is_valid_symbol(Token *tok);

// ********** defined in literal.c **********

// literal (immediate) value that is hardcoded in the source file
// binary and hexadecimal literal must be represented in two's complement
// while literal in decimal can use the negative sign
enum LiteralType {
    LIT_RES,    // invalid type (reserved for the trolls)
    LIT_HEX,    // hexadecimal type
    LIT_BIN,    // binary type
    LIT_DEC,    // decimal type
};

// parses into interger value from literal token
uint16_t parse_literal(Token *tok);
// determines the literal type of the literal token
uint8_t get_literal_type(Token *tok);

// ********** defined in assemble.c **********

// // determines address to the first instruction
// uint16_t find_orig(FileList *fl, InputBuf *ibuf, OutputBuf *obuf, uint32_t *p_lidx);
// // checks for tokens' validity, generates symbol table and input buffer in the meantime
// uint8_t validate_line(InputBuf *ibuf, SymTab *st, Line *ln, uint16_t *p_addr);
// // translates assembly instructions to machine code, generates output table in the meantime
// uint8_t generate_machine_code(OutputBuf *obuf, SymTab *st, Line *ln, uint16_t *p_addr);

// assemble an assembly file
void assemble(InputBuf *ibuf, OutputBuf *obuf, SymTab *st, FileList *fl, char *filename);

#endif