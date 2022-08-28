#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <windows.h>

#include "dict.h"

static uint64_t fnv_hash(const char *key, uint64_t len) {
    static const uint64_t fnv_basis = 0xcbf29ce484222325;
    static const uint64_t fnv_prime = 0x100000001b3;

    uint64_t h = fnv_basis;
    for (uint64_t i = 0; i < len; i++) {
        h = (h * fnv_prime) ^ key[i];
    }
    return h;
}

static DictEntry *dict_new_entry(const char *key, const void *val, uint64_t valsz) {
    uint64_t keylen = strlen(key);
    uint64_t hash = fnv_hash(key, keylen);

    ++keylen;
    uint64_t keysz = keylen + ((sizeof(void *) - keylen) % sizeof(void *));

    DictEntry *e = malloc(sizeof *e + keysz + valsz);
    if (e == NULL) {
        return NULL;
    }

    e->hash = hash;
    e->next = NULL;
    e->key = (char *)(e + 1);
    memcpy(e->key, key, keylen);
    e->val = e->key + keysz;
    memcpy(e->val, val, valsz);

    return e;
}

static uint64_t dict_bucket_index(DictTable *t, uint64_t hash) {
    return hash % t->bucket_cnt;
}

static void dict_add_entry(DictTable *t, DictEntry *e) {
    uint64_t i = dict_bucket_index(t, e->hash);
    e->next = t->buckets[i];
    t->buckets[i] = e;
}

static DictEntry *dict_find_entry(DictTable *t, const char *key) {
    if (t->bucket_cnt == 0 || t->entry_cnt == 0) {
        return NULL;
    }

    uint64_t i = dict_bucket_index(t, fnv_hash(key, strlen(key)));
    DictEntry *e = t->buckets[i];

    while (e != NULL) {
        if (strcmp(key, e->key) == 0) {
            return e;
        }
        e = e->next;
    }
    return NULL;
}

void *dict_get_value(DictTable *t, const char *key) {
    DictEntry *e = dict_find_entry(t, key);
    if (e == NULL) {
        return NULL;
    } else {
        return e->val;
    }
}

static int dict_rehash(DictTable *t, uint64_t bucket_cnt) {
    DictEntry *q = NULL;
    DictEntry *cur;
    DictEntry *next;

    for (uint64_t i = 0; i < t->bucket_cnt; i++) {
        cur = t->buckets[i];
        while (cur != NULL) {
            next = cur->next;
            cur->next = q;
            q = cur;
            cur = next;
        }
    }

    DictEntry **buf = realloc(t->buckets, bucket_cnt * sizeof *buf);
    if (buf == NULL) {
        return -1;
    }

    t->buckets = buf;
    t->bucket_cnt = bucket_cnt;
    memset(t->buckets, 0, bucket_cnt * sizeof *buf);

    cur = q;
    while (cur != NULL) {
        next = cur->next;
        dict_add_entry(t, cur);
        cur = next;
    }

    return 0;
}

void dict_clear_table(DictTable *t) {
    DictEntry *cur;
    DictEntry *next;

    for (uint64_t i = 0; i < t->bucket_cnt; i++) {
        cur = t->buckets[i];
        while (cur != NULL) {
            next = cur->next;
            free(cur);
            cur = next;
        }
    }
    free(t->buckets);
    memset(t, 0, sizeof *t);
}

int dict_set_entry(DictTable *t, const char *key, const void *val, uint64_t valsz) {
    DictEntry *dupe = dict_find_entry(t, key);
    if (dupe != NULL) {
        memcpy(dupe->val, val, valsz);
        return 0;
    }

    if (t->entry_cnt >= 5 * t->bucket_cnt) {
        uint64_t reserved = (t->bucket_cnt == 0 ? 1 : t->bucket_cnt << 1);
        if (reserved > DICT_MAX_BUCKET_CNT) {
            return -1;
        }
        if (dict_rehash(t, reserved) != 0) {
            return -1;
        }
    }

    DictEntry *e = dict_new_entry(key, val, valsz);
    if (e == NULL) {
        return -1;
    }

    dict_add_entry(t, e);
    t->entry_cnt++;

    return 0;
}

void dict_delete_entry(DictTable *t, const char *key) {
    if (t->bucket_cnt == 0 || t->entry_cnt == 0) {
        return;
    }

    uint64_t i = dict_bucket_index(t, fnv_hash(key, strlen(key)));
    DictEntry *e = t->buckets[i];
    DictEntry *prev = NULL;

    while (e != NULL) {
        if (strcmp(key, e->key) == 0) {
            break;
        }
        prev = e;
        e = e->next;
    }

    if (e == NULL) {
        return;
    } else if (prev == NULL) {
        t->buckets[i] = e->next;
    } else {
        prev->next = e->next;
    }

    t->entry_cnt--;
    free(e);
}

uint64_t dict_table_size(DictTable *t) {
    return t->entry_cnt;
}

unsigned int rand_seed(void) {
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME sys_time;
    FILETIME file_time;
    GetSystemTime(&sys_time);
    SystemTimeToFileTime(&sys_time, &file_time);

    uint64_t sec = (uint64_t)(file_time.dwLowDateTime);
    sec |= (uint64_t)(file_time.dwHighDateTime) << 32;
    uint64_t tv_sec = (uint64_t)((sec - EPOCH) / 10000000L);
    uint64_t tv_usec = (uint64_t)(sys_time.wMilliseconds * 1000);

    return (unsigned int)((tv_sec ^ tv_usec) | 1);
}

unsigned int rand_gen(unsigned int *seed) {
    return (*(seed) *= 3) >> 1;
}

const DictEntry *dict_rand_entry(DictTable *t) {
    if (t->entry_cnt == 0 || t->bucket_cnt == 0) {
        return NULL;
    }

    unsigned int seed = rand_seed();
    DictEntry *e = NULL;
    while (e == NULL) {
        e = t->buckets[rand_gen(&seed) % t->bucket_cnt];
    }

    int n = 2;
    DictEntry *cur = e->next;
    while (cur != NULL) {
        unsigned int r = rand_gen(&seed) % n;
        if (r == 0) {
            e = cur;
        }
        n++;
        cur = cur->next;
    }
    return e;
}