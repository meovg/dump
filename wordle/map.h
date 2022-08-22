#ifndef MAP_H
#define MAP_H

#include <string.h>
#include <inttypes.h>

typedef struct HashEntry HashEntry;
typedef struct HashTable HashTable;

#define MapOf(V) \
struct { \
  HashTable table; \
  V val; \
  V *ref; \
}

#define map_init(map) \
  (memset(&((map)->table), 0, sizeof (map)->table))

#define map_set(map, key, value) do { \
  (map)->val = value; \
  map_ht_set(&((map)->table), key, &((map)->val), sizeof (map)->val); \
} while (0)

#define map_get(map, key) \
  ((map)->ref = map_ht_get(&((map)->table), key)) 

#define map_delete(map, key) \
  (map_ht_delete(&((map)->table), key))

#define map_clear(map) \
  (map_ht_clear(&((map)->table)))

#define map_size(map) \
  (map_ht_size(&((map)->table)))

#define map_rand(map) \
  (map_ht_rand(&((map)->table)))

struct HashEntry {
  uint64_t hash;
  char *key;
  void *value;
  HashEntry *next;
};

struct HashTable {
  HashEntry** buckets;
  uint64_t entrysize;
  uint64_t bucketsize;
};

#define DICT_HT_MAX_ENTRY_SIZE  (UINT64_MAX)
#define DICT_HT_MAX_BUCKET_SIZE (UINT64_MAX >> 2)

int map_ht_set(HashTable* table, const char *key, const void *value, uint64_t valsize);

void *map_ht_get(const HashTable *table, const char *key);

void map_ht_delete(HashTable *table, const char *key);

void map_ht_clear(HashTable *table);

uint64_t map_ht_size(const HashTable *table);

const HashEntry *map_ht_rand(const HashTable *table);

#endif /* MAP_H */