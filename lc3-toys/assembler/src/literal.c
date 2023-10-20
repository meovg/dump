#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "common.h"

// refer to https://en.wikipedia.org/wiki/Two%27s_complement#Converting_from_two's_complement_representation
static uint16_t parse_two_complement(const char *s) {
    static const int N = 16;
    static char seq[17];

    size_t len = strlen(s);
    size_t i = len - 1;
    size_t j = 0;

    // copy bit sequence from the bottom up for easier looking calculation
    while (j < len && j < N) {
        seq[j] = s[i] - '0';
        j++;
        i--;
    }

    uint16_t res = 0;
    for (j = 0; j <= N-2; j++) {
        res += seq[j] * (1 << j);
    }

    res -= seq[N-1] * (1 << (N-1));
    return res;
}

// gets decimal value of hex digit
static int8_t hex_index(char c) {
    static const char hex[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    for (int8_t i = 0; i < 16; i++) {
        if (i < 10 && c == hex[i])
            return i;
        else if (i >= 10 && (c == hex[i] || c == hex[i] + 32))
            return i;
    }
    return -1;
}

// calculates the two's complement literal represented in hex code
static uint16_t parse_hex(const char *s) {
    static const int N = 4;
    static char seq[5];
    static char bin[17];
    char *tmp;

    memset(seq, 0, 5 * sizeof(char));
    memset(bin, 0, 17 * sizeof(char));

    // move to the start of actual number in literal
    char *num = strchr(s, 'x');
    if (num == NULL)
        num = strchr(s, 'X');
    num++;

    size_t len = strlen(num);
    size_t i = len - 1;
    size_t j = 0;
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
        memcpy(tmp, binseq[(size_t)seq[j]], 4 * sizeof(char));
        j++;
    }
    return parse_two_complement(bin);
}

static uint16_t parse_bin(char *s) {
    char *num = strchr(s, 'b');
    if (num == NULL)
        num = strchr(s, 'B');
    num++;
    return parse_two_complement(num);
}

static uint16_t parse_dec(char *s) {
    int8_t neg = 0;
    int16_t res = 0;
    size_t i = 0;

    char *num = strchr(s, '#');
    if (num == NULL)
        num = s;
    else
        num++;

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

// parses the token containing the literal
uint16_t parse_literal(Token *tok) {
    char *s = tok->str;
    int16_t val = 0;
    uint8_t type = tok->subtype;

    if (type == LIT_HEX)
        val = parse_hex(s);
    else if (type == LIT_BIN)
        val = parse_bin(s);
    else if (type == LIT_DEC)
        val = parse_dec(s);
    return val;
}

// returns the literal type of the presumably "literal" token
uint8_t get_literal_type(Token *tok) {
    char *s = tok->str;
    char *lit;

    if (s[0] == 'x' || s[0] == 'X') {
        // hexadecimal type: x$$$$ or X$$$$ format (no negative sign)
        // $ in range [0..9, A..F]
        lit = s + 1;
        while (*lit != '\0') {
            if (hex_index(*lit) == -1)
                return LIT_RES;
            lit++;
        }
        return LIT_HEX;
    } else if (s[0] == 'b' || s[0] == 'B') {
        // binary type: b$$...$ or B$$...$ format (no negative sign)
        // $ is either 0 or 1
        lit = s + 1;
        while (*lit != '\0') {
            if (*lit != '0' && *lit != '1')
                return LIT_RES;
            lit++;
        }
        return LIT_BIN;
    } else if (s != NULL) {
        // decimal type: #$$$..$ or $$$..$ format (1 negative sign allowed)
        lit = strchr(s, '#');

        if (lit == NULL)
            lit = s;
        else
            lit++;

        if (*lit == '-')
            lit++;

        while (*lit != '\0') {
            if (!isdigit(*lit))
                return LIT_RES;
            lit++;
        }
        return LIT_DEC;
    }
    return LIT_RES;
}