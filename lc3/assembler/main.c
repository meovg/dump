#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ===== token ===== */

#define MAX_TOKEN_NUM 7
#define MAX_TOKEN_LEN 1025
#define MAX_LEN 7175

typedef struct {
    uint16_t size;
    char *raw;
} Token;

typedef struct {
    uint8_t size;
    Token **raw;
} TokenBuf;

char get_escape_value(char c)
{
    static const char escape_chars[] = {'\'', '\"', '\?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'};
    static const char escape_vals[] = {0x27, 0x22, 0x3F, 0x5C, 0x07, 0x08, 0x0C, 0x0A, 0x0D, 0x09, 0x0B};

    for (uint8_t i = 0; i < 11; i++) {
        if (c == escape_chars[i]) return escape_vals[i];
    }
    return c;
}

void token_copy(Token *dst, const Token *src)
{
    dst->size = src->size;
    dst->raw = malloc((dst->size + 1) * sizeof(char));
    memcpy(dst->raw, src->raw, (dst->size + 1) * sizeof(char));
}

Token *token_from_string(const char *line, uint16_t *index_ptr)
{
    uint16_t i = *index_ptr;
    uint16_t j = 0;
    char tmp[MAX_TOKEN_LEN];

    while (line[i] != '\0') {
        if (line[i] == '\"') {
            break;
        } else if (line[i] == '\\') {
            tmp[j] = get_escape_value(line[++i]);
            j++;
            i++;
        } else {
            tmp[j] = line[i];
            j++;
            i++;
        }
    }

    Token *t = malloc(sizeof *t);
    t->size = j + 1;
    t->raw = malloc((t->size + 1) * sizeof(char));
    for (uint16_t k = 0; k < j; k++) {
        t->raw[k] = tmp[k];
    }
    t->raw[j] = '\0';

    *index_ptr = i;
    return t;
}

int8_t is_eow(char c)
{
    int8_t eow = 0;
    static const char eowchars[] = {0, 13, '\t', '\n', ' ', ',', ';'};

    for (uint8_t i = 0; i < 7; i++) {
        if (c == eowchars[i]) eow++;
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

    Token *t = malloc(sizeof *t);
    t->size = j + 1;
    t->raw = malloc((t->size + 1) * sizeof(char));
    for (uint8_t k = 0; k < j; k++) {
        t->raw[k] = tmp[k];
    }
    t->raw[j] = '\0';

    *index_ptr = i - 1;
    return t;
}

int8_t is_eol(char c)
{
    int8_t eol = 0;
    static const char eolchars[] = {0, 10, 13, 59};

    for (uint8_t i = 0; i < 4; i++) {
        if (c == eolchars[i]) eol++;
    }

    return eol;
}

TokenBuf *tokenize(const char *line)
{
    TokenBuf *buf = malloc(sizeof *buf);
    buf->size = 0;
    buf->raw = malloc(sizeof(Token *));

    uint8_t new_token = 0;
    int8_t is_sep = 0;
    int8_t is_quote = 0;
    int8_t prev_is_sep = 1;

    for (uint16_t i = 0; !is_eol(line[i]); i++) {
        is_sep = (line[i] == ' ' || line[i] == '\t' || line[i] == ',');
        is_quote = (line[i] == '\"');
        
        if (is_quote) {
            i++;
            buf->raw[buf->size] = token_from_string(line, &i);
            buf->size++;
            new_token = 1;
        } else if (prev_is_sep && !is_sep) {
            buf->raw[buf->size] = token_from_word(line, &i);
            buf->size++;
            is_sep = 1;
            new_token = 1;
        }

        if (new_token) {
            buf->raw = realloc(buf->raw, (buf->size + 1) * sizeof(Token *));
            new_token = 0;
        }

        prev_is_sep = is_sep;
    }

    return buf;
}

void token_free(Token *t)
{
    if (t) {
        if (t->raw) free(t->raw);
        free(t);
    }
}

void token_buffer_free(TokenBuf *buf)
{
    if (!buf) return;
    if (buf->raw) {
        for (uint8_t i = 0; i < buf->size; i++) {
            if (buf->raw[i]->size) {
                token_free(buf->raw[i]);
            }
        }
        free(buf->raw);
    }
    free(buf);
}

/* ===== lines ===== */

typedef struct {
    TokenBuf *buf;
    uint32_t ln;
} Line;

typedef struct {
    uint32_t size;
    uint32_t cap;
    Line **raw;
} LineArr;

#define LINEARR_DEFAULT_SIZE 200

LineArr *linearr_new(void)
{
    LineArr *arr = malloc(sizeof *arr);
    arr->cap = LINEARR_DEFAULT_SIZE;
    arr->size = 0;
    arr->raw = malloc(arr->cap * sizeof(Line *));
    return arr;
}

void linearr_set(LineArr *arr, TokenBuf *buf, uint32_t ln)
{
    if (arr->size == arr->cap) {
        arr->cap *= 2;
        arr->raw = realloc(arr->raw, arr->cap * sizeof(Line *));
    }
    arr->raw[arr->size] = malloc(sizeof(Line *));
    arr->raw[arr->size]->buf = buf;
    arr->raw[arr->size]->ln = ln;
    arr->size++;
}

Line *linearr_get(LineArr *arr, uint32_t ind)
{
    if (ind >= arr->size) return NULL;
    return arr->raw[ind];
}

void linearr_free(LineArr *arr)
{
    if (!arr) return;
    for (uint32_t i = 0; i < arr->size; i++) {
        token_buffer_free(arr->raw[i]->buf);
        free(arr->raw[i]);
    }
    free(arr->raw);
    free(arr);
}

/* ===== file buffer ===== */

typedef struct {
    uint16_t *addrs;
    uint16_t *instrs;
    uint32_t *lns;
    uint16_t size;
    uint16_t cap;
} FileBuf;

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

/* ===== files ===== */

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

uint8_t filelist_open(FileList *fl, char *filename)
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

void filelist_close(FileList *fl)
{
    if (!fl) return;
    if (fl->src) fclose(fl->src);
    if (fl->sym) fclose(fl->sym);
    if (fl->obj) fclose(fl->obj);
    if (fl->lst) fclose(fl->lst);
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

void write_obj(FileBuf *buf, FILE *obj)
{
    uint16_t byte_cnt = buf->size;
    uint8_t tmp[byte_cnt];

    for (uint16_t i = 0; i < buf->size; i++) {
        tmp[i * 2] = buf->instrs[i] >> 8; // high byte
        tmp[i * 2 + 1] = buf->instrs[i] & 0xff; // low byte
    }

    fwrite(tmp, sizeof(uint8_t), byte_cnt, obj);
}

void write_lst(FileBuf *buf, FILE *lst, FILE *src)
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