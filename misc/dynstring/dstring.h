#ifndef DSTRING_H
#define DSTRING_H

#include <inttypes.h>

enum {
  E_DSTRING_SUCCESS,
  E_DSTRING_NGPTR,
  E_DSTRING_NGVAL,
  E_DSTRING_NALLOC,
  E_DSTRING_NGSIZE,
  E_DSTRING_OUTRANGE,
  E_DSTRING_FAIL
};

const char *dstring_errname(int err);

#define dstring_error(reason, err) do { \
  fprintf(stderr, "ERROR: %s: in %s at line %d\n%s", \
          dstring_errname(err), __FILE__, __LINE__, reason); \
  fflush(stderr); \
} while (0)

typedef union Dstring Dstring;

union Dstring {
  struct {
    char data[23];
    uint8_t size;
  } s;
  struct {
    char *data;
    uint64_t size;
    uint64_t capacity;
  } l;
};

#define STRING_LONG_MAXCAP        ((UINT64_MAX >> 1) - 1)
#define STRING_SHORT_MAXCAP       (23)

#define dstring_is_short(str)     ((str)->s.size >> 7)
#define dstring_short_size(str)   ((str)->s.size & 0x7f)

Dstring *dstring_create(void);

Dstring *dstring_from_cstr(const char *src, uint64_t length);

void dstring_clear(Dstring *str);

void dstring_free(Dstring *str);

void dstring_shrink_opt(Dstring *str);

void dstring_init(Dstring *str);

void dstring_assign(Dstring *str, const Dstring *value);

char dstring_get_at(const Dstring *str, uint64_t pos);

int dstring_set_at(Dstring *str, uint64_t pos, char value);

uint64_t dstring_size(const Dstring *str);

uint64_t dstring_capacity(const Dstring *str);

const char *dstring_to_cstr(const Dstring *str);

void dstring_insert(Dstring *str, uint64_t pos, const char *value, uint64_t length);

void dstring_append(Dstring *str, const char * value, uint64_t length);

void dstring_prepend(Dstring *str, const char *value, uint64_t length);

void dstring_concat(Dstring *str, const Dstring *rhs);

void dstring_erase(Dstring *str, uint64_t pos, uint64_t length);

Dstring *dstring_substr(const Dstring *str, uint64_t pos, uint64_t length);

#endif /* DSTRING_H */