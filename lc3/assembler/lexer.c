#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

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

void copy_token(Token *dst, Token *src)
{
    dst->size = src->size;
    dst->raw = malloc((dst->size + 1) * sizeof(char));
    memcpy(dst->raw, src->raw, (dst->size + 1) * sizeof(char));
}

Token *token_from_string(char *line, uint16_t *index_ptr)
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

int8_t is_end_of_word(char c)
{
    int8_t eow = 0;
    static const char eowchars[] = {0, 13, '\t', '\n', ' ', ',', ';'};

    for (uint8_t i = 0; i < 7; i++) {
        if (c == eowchars[i]) eow++;
    }
    return eow;
}

Token *token_from_word(char *line, uint16_t *index_ptr)
{
    uint8_t i = *index_ptr;
    uint8_t j = 0;
    char tmp[MAX_TOKEN_LEN];

    while (!is_end_of_word(line[i])) {
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

int8_t is_end_of_line(char c)
{
    int8_t eol = 0;
    static const char eolchars[] = {0, 10, 13, 59};

    for (uint8_t i = 0; i < 4; i++) {
        if (c == eolchars[i]) eol++;
    }

    return eol;
}

TokenBuf *tokenize(char *line)
{
    TokenBuf *buf = malloc(sizeof *buf);
    buf->size = 0;
    buf->raw = malloc(sizeof(Token *));

    uint8_t new_token = 0;
    int8_t is_sep = 0;
    int8_t is_quote = 0;
    int8_t prev_is_sep = 1;

    for (uint16_t i = 0; !is_end_of_line(line[i]); i++) {
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

void free_token(Token *t)
{
    if (t) {
        if (t->raw) free(t->raw);
        free(t);
    }
}

void free_token_buffer(TokenBuf *buf)
{
    if (!buf) return;
    if (buf->raw) {
        for (uint8_t i = 0; i < buf->size; i++) {
            if (buf->raw[i]->size) {
                free_token(buf->raw[i]);
            }
        }
        free(buf->raw);
    }
    free(buf);
}

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

void linearr_set(LineArr *arr, TokenBuffer *buf, uint32_t ln)
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

Line *linearr_get(LineArr *arr, uint32_t index)
{
    if (index >= arr->size) return NULL;
    return arr->raw[index]
}

void linearr_free(LineArr *arr)
{
    if (!arr) return;
    for (uint32_t i = 0; i < arr->size; i++) {
        free_token_buffer(arr->raw[i]->buf);
        free(arr->raw[i]);
    }
    free(arr->raw);
    free(arr);
}