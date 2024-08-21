#ifndef DICT_H
#define DICT_H

#include <string.h>
#include <inttypes.h>

typedef struct DictEntry DictEntry;
typedef struct DictTable DictTable;

#define dict_of(V) \
struct { \
    DictTable table; \
    V val; \
    V *ref; \
}

#define dict_init(d) (memset(&((d)->table), 0, sizeof (d)->table))

#define dict_set(d, _key, _val) do { \
    (d)->val = _val; \
    dict_set_entry(&((d)->table), _key, &((d)->val), sizeof (d)->val); \
} while (0)

#define dict_get(d, _key) ((d)->ref = dict_get_value(&((d)->table), _key)) 
#define dict_delete(d, _key) (dict_delete_entry(&((d)->table), _key))
#define dict_clear(d) (dict_clear_table(&((d)->table)))
#define dict_size(d) (dict_table_size(&((d)->table)))
#define dict_rand(d) (dict_rand_entry(&((d)->table)))

struct DictEntry {
    uint64_t hash;
    char *key;
    void *val;
    DictEntry *next;
};

struct DictTable {
    DictEntry** buckets;
    uint64_t entry_cnt;
    uint64_t bucket_cnt;
};

#define DICT_MAX_ENTRY_CNT  (UINT64_MAX)
#define DICT_MAX_BUCKET_CNT (UINT64_MAX >> 2)

int dict_set_entry(DictTable *t, const char *key, const void *val, uint64_t valsz);
// static DictEntry *dict_find_entry(DictTable *t, const char *key);
void dict_delete_entry(DictTable *t, const char *key);

void *dict_get_value(DictTable *t, const char *key);

void dict_clear_table(DictTable *t);

uint64_t dict_table_size(DictTable *t);
const DictEntry *dict_rand_entry(DictTable *t);

#endif /* MAP_H */