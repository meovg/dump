#include <stdlib.h>
#include <string.h>

#include "common.h"

// gets a token of substring until the end of a line or a quote
// used for lexing string literals 
Token *token_get_until_eol(uint32_t lidx, const char *str, uint16_t *p_idx) {
    uint16_t i = *p_idx;
    uint16_t j = 0;
    static char tmp[MAX_TOKEN_LEN + 1];

    // load characters to be included in the token in a temporary array
    while (str[i] != '\0' && str[i] != '\"') {
        if (str[i] == '\\') {
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

    Token *tok = mymalloc(sizeof *tok);
    tok->size = j + 1;
    tok->str = mymalloc(tok->size * sizeof(char));
    tok->lidx = lidx;

    for (uint16_t k = 0; k < j; k++)
        tok->str[k] = tmp[k];
    tok->str[j] = '\0';

    // this is of string type, don't call token_validate() after this
    tok->type = TK_STR;

    return tok;
}

// validates token type
// only use this for token returned from get_token_until_eow()
static void get_word_token_type(Token *tok) {
    if ((tok->subtype = get_opcode_type(tok)) != -1)
        tok->type = TK_OPCODE;
    else if ((tok->subtype = get_pseudo_type(tok)) != -1)
        tok->type = TK_PSEUDO;
    else if ((tok->subtype = get_register_type(tok)) != -1)
        tok->type = TK_REGISTER;
    else if ((tok->subtype = get_literal_type(tok)) != LIT_RES)
        tok->type = TK_LITERAL;
    else if (is_valid_symbol(tok))
        tok->type = TK_SYMBOL;
    else
        tok->type = TK_INVALID;
}

// gets a token of substring until a word separator is found
// used for lexing symbols, opcodes, psuedo-ops and operands...
Token *token_get_until_eow(uint32_t lidx, const char *str, uint16_t *p_idx) {
    uint8_t i = *p_idx;
    uint8_t j = 0;
    static char tmp[MAX_TOKEN_LEN + 1];

    while (!is_eow(str[i])) {
        tmp[j] = str[i];
        j++;
        i++;
    }

    // index should be at the last character in a token
    // so that tokenize() can check if the next character is a separator
    *p_idx = i - 1;

    Token *tok = mymalloc(sizeof *tok);
    tok->size = j + 1;
    tok->str = mymalloc(tok->size * sizeof(char));
    tok->lidx = lidx;

    for (uint8_t k = 0; k < j; k++)
        tok->str[k] = tmp[k];
    tok->str[j] = '\0';

    get_word_token_type(tok);

    return tok;
}

Token *token_copy(Token *src) {
    Token *dst = mymalloc(sizeof(*dst));

    dst->type = src->type;
    dst->subtype = src->subtype;
    dst->size = src->size;
    dst->lidx = src->lidx;

    dst->str = mymalloc(dst->size * sizeof(char));
    memcpy(dst->str, src->str, dst->size);

    return dst;
}

// deallocates the token
void token_del(Token *tok) {
    if (tok) {
        if (tok->str)
            free(tok->str);
        free(tok);
    }
}