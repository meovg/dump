#include <stdlib.h>

#include "common.h"

// breaks down an instruction line into several chunks of strings (tokens)
Line *line_new(uint32_t lidx, const char *str) {
    int8_t need_token = 0;
    int8_t is_sep = 0;
    int8_t prev_is_sep = 1;

    Line *ln = mymalloc(sizeof *ln);
    ln->tokcnt = 0;
    ln->tokens = mymalloc(sizeof(Token *));

    for (uint16_t i = 0; !is_eol(str[i]); i++) {
        is_sep = (str[i] == ' ' || str[i] == '\t' || str[i] == ',');

        if (str[i] == '\"') {
            i++;
            ln->tokens[ln->tokcnt] = token_get_until_eol(lidx, str, &i);
            ln->tokcnt++;
            need_token = 1;
        } else if (prev_is_sep && !is_sep) {
            ln->tokens[ln->tokcnt] = token_get_until_eow(lidx, str, &i);
            ln->tokcnt++;
            is_sep = 1;
            need_token = 1;
        }

        // the limit is only 7 so no need to realloc twice the current capacity
        if (need_token) {
            ln->tokens = myrealloc(ln->tokens, (ln->tokcnt + 1) * sizeof(Token *));
            need_token = 0;
        }

        prev_is_sep = is_sep;
    }
    return ln;
}

// deallocates a Line object and the tokens contained inside
void line_del(Line *ln) {
    if (!ln)
        return;

    for (uint8_t i = 0; i < ln->tokcnt; i++) {
        if (ln->tokens[i]->size)
            token_del(ln->tokens[i]);
    }
    free(ln->tokens);
    free(ln);
}