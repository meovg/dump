#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "map.h"

#define FNV_OFFSET_BASIS    (0xcbf29ce484222325)
#define FNV_PRIME           (0x100000001b3)

static uint64_t fnv_hash(const char *key, uint64_t length) {
    uint64_t hash = FNV_OFFSET_BASIS;
    for (uint64_t i = 0; i < length; i++) {
        hash = (hash * FNV_PRIME) ^ key[i];
    }
    return hash;
}

static HashEntry *map_ht_new_entry(const char *key, const void *value, uint64_t n) {
    uint64_t keylen = strlen(key);
    uint64_t hash = fnv_hash(key, keylen);

    ++keylen;
    uint64_t keysize = keylen + ((sizeof(void *) - keylen) % sizeof(void *));

    HashEntry *entry = malloc(sizeof *entry + keysize + n);
    if (entry == NULL) {
        return NULL;
    }

    entry->hash = hash;
    entry->next = NULL;
    entry->key = (char *)(entry + 1);
    memcpy(entry->key, key, keylen);
    entry->value = entry->key + keysize;
    memcpy(entry->value, value, n);

    return entry;
}

static uint64_t map_ht_bucket_index(const HashTable *table, uint64_t hash) {
    return hash % table->bucketsize;
}

static void map_ht_add_into_bucket(HashTable *table, HashEntry *entry) {
    uint64_t idx = map_ht_bucket_index(table, entry->hash);
    entry->next = table->buckets[idx];
    table->buckets[idx] = entry;
}

static HashEntry *map_ht_get_entry(const HashTable *table, const char *key) {
    if (table->bucketsize == 0 || table->entrysize == 0) {
        return NULL;
    }

    uint64_t idx = map_ht_bucket_index(table, fnv_hash(key, strlen(key)));
    HashEntry *ref = table->buckets[idx];

    while (ref != NULL) {
        if (strcmp(key, ref->key) == 0) {
            return ref;
        }
        ref = ref->next;
    }
    return NULL;
}

void *map_ht_get(const HashTable *table, const char *key) {
    HashEntry *ref = map_ht_get_entry(table, key);
    if (ref == NULL) {
        return NULL;
    } else {
        return ref->value;
    }
}

static int map_ht_rehash(HashTable *table, uint64_t newsize) {
    HashEntry *entry_list = NULL;
    HashEntry *now;
    HashEntry *next;

    for (uint64_t i = 0; i < table->bucketsize; i++) {
        now = table->buckets[i];
        while (now != NULL) {
            next = now->next;
            now->next = entry_list;
            entry_list = now;
            now = next;
        }
    }

    HashEntry **buckets_buf = realloc(table->buckets, newsize * sizeof *buckets_buf);
    if (buckets_buf == NULL) {
        return -1;
    }

    table->buckets = buckets_buf;
    table->bucketsize = newsize;
    memset(table->buckets, 0, newsize * sizeof *buckets_buf);

    now = entry_list;
    while (now != NULL) {
        next = now->next;
        map_ht_add_into_bucket(table, now);
        now = next;
    }
    return 0;
}

void map_ht_clear(HashTable *table) {
    HashEntry *now;
    HashEntry *next;
    for (uint64_t i = 0; i < table->bucketsize; i++) {
        now = table->buckets[i];
        while (now != NULL) {
            next = now->next;
            free(now);
            now = next;
        }
    }
    free(table->buckets);
    memset(table, 0, sizeof *table);
}

int map_ht_set(HashTable *table, const char *key, const void *value, uint64_t n) {
    HashEntry *duplicate = map_ht_get_entry(table, key);
    if (duplicate != NULL) {
        memcpy(duplicate->value, value, n);
        return 0;
    }

    if (table->entrysize >= 4 * table->bucketsize) {
        uint64_t new_bucketsize = (table->bucketsize == 0 ? 1 : table->bucketsize << 1);
        if (new_bucketsize > DICT_HT_MAX_BUCKET_SIZE) {
            return -1;
        }
        if (map_ht_rehash(table, new_bucketsize) != 0) {
            return -1;
        }
    }

    HashEntry *new_entry = map_ht_new_entry(key, value, n);
    if (new_entry == NULL) {
        return -1;
    }

    map_ht_add_into_bucket(table, new_entry);
    table->entrysize++;
    return 0;
}

void map_ht_delete(HashTable *table, const char *key) {
    if (table->bucketsize == 0 || table->entrysize == 0) {
        return;
    }

    uint64_t idx = map_ht_bucket_index(table, fnv_hash(key, strlen(key)));
    HashEntry *ref = table->buckets[idx];
    HashEntry *prev = NULL;

    while (ref != NULL) {
        if (strcmp(key, ref->key) == 0) {
            break;
        }
        prev = ref;
        ref = ref->next;
    }

    if (ref == NULL) {
        return;
    } else if (prev == NULL) {
        table->buckets[idx] = ref->next;
    } else {
        prev->next = ref->next;
    }
    table->entrysize--;
    free(ref);
}

uint64_t map_ht_size(const HashTable *table) {
    return table->entrysize;
}