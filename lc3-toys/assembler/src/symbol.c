#include <stdlib.h>
#include <string.h>

#include "common.h"

// made-up numeral system for hash
static uint32_t ch_base(char c) {
    // numeric (0, 9), alphabetic uppercase (10, 25), lowercase (26, 51), underscore (52)
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    return 52;
}

static uint32_t hashf(Token *tok) {
    uint32_t h = 0;

    for (uint16_t i = 0; i < tok->size; i++)
        h = h * 53 + ch_base(tok->str[i]);

    // taken from gzm55/hash-garage repository, file nmhash.h
    h ^= h >> 15;
    h *= 0xd168aaad;
    h ^= h >> 15;
    h *= 0xaf723597;
    h ^= h >> 15;
    return h;
}

static Symbol *symbol_new(Token *tok, uint16_t addr) {
    Symbol *sym = mymalloc(sizeof (*sym));

    sym->hash = hashf(tok);
    sym->addr = addr;
    sym->next = NULL;

    // allocate and copy the contents from label token to sym->key
    sym->key = token_copy(tok);

    return sym;
}

SymTab *symtab_new(void) {
    SymTab *st = mymalloc(sizeof (*st));

    st->size = 0;
    st->bktcnt = SYMTAB_DEFAULT_BKTCNT;
    st->buckets = mymalloc(st->bktcnt * sizeof(Symbol *));

    // bucket pointers are initialized to NULL to avoid segmentation fault
    // while traversing along the bucket elements
    memset(st->buckets, 0, st->bktcnt * sizeof(Symbol *));

    return st;
}

// adds the already initialized Symbol object to the symbol table
static void symtab_add_sym(SymTab *st, Symbol *sym) {
    uint32_t idx = sym->hash % st->bktcnt;
    sym->next = st->buckets[idx];
    st->buckets[idx] = sym;
}

Symbol *symtab_find(SymTab *st, Token *tok) {
    if (st->bktcnt == 0 || st->size == 0)
        return NULL;

    uint32_t idx = hashf(tok) % st->bktcnt;
    Symbol *bucket_iter = st->buckets[idx];

    while (bucket_iter != NULL) {
        if (strcmp(tok->str, bucket_iter->key->str) == 0)
            return bucket_iter;
        bucket_iter = bucket_iter->next;
    }
    return NULL;
}

// checks if an instruction address has a symbol or return NULL
// this routine does brute-force search instead
Symbol *symtab_find_by_value(SymTab *st, uint16_t addr) {
    for (uint16_t i = 0; i < st->bktcnt; i++) {
        Symbol *bucket_iter = st->buckets[i];

        while (bucket_iter != NULL) {
            if (bucket_iter->addr == addr)
                return bucket_iter;
            bucket_iter = bucket_iter->next;
        }
    }
    return NULL;
}

// this function allocates more buckets and places existing entries
// into buckets based on the precomputed hash value 
static void symtab_rehash(SymTab *st, uint32_t new_bktcnt) {
    Symbol *l_sym = NULL;
    Symbol *cur;
    Symbol *next;

    // link existing entries into a single temporary linked list
    for (uint32_t i = 0; i < st->bktcnt; i++) {
        cur = st->buckets[i];
        while (cur != NULL) {
            next = cur->next;
            cur->next = l_sym;
            l_sym = cur;
            cur = next;
        }
    }

    // reallocate buckets
    Symbol **new_buckets = myrealloc(st->buckets, new_bktcnt * sizeof *new_buckets);

    st->buckets = new_buckets;
    st->bktcnt = new_bktcnt;
    // set all bucket pointers to NULL to avoid segmentation fault
    memset(st->buckets, 0, st->bktcnt * sizeof *new_buckets);

    // place existing entries into new buckets
    cur = l_sym;
    while (cur != NULL) {
        next = cur->next;
        symtab_add_sym(st, cur);
        cur = next;
    }
}

void symtab_add(SymTab *st, Token *tok, uint16_t addr) {
    Symbol *sym = symtab_find(st, tok);

    if (sym != NULL) {
        sym->addr = addr;
        return;
    }

    if (st->size >= 5 * st->bktcnt) {
        uint32_t new_bktcnt = st->bktcnt << 1;
        symtab_rehash(st, new_bktcnt);
    }

    sym = symbol_new(tok, addr);
    symtab_add_sym(st, sym);
    st->size++;
}

void symtab_del(SymTab *st) {
    Symbol *cur;
    for (uint32_t i = 0; i < st->bktcnt; i++) {
        cur = st->buckets[i];
        while (cur != NULL) {
            Symbol *next = cur->next;
            token_del(cur->key);
            free(cur);
            cur = next;
        }
    }
    free(st->buckets);
    free(st);
}