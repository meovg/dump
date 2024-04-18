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

#define STRING_LONG_MAXCAP          ((UINT64_MAX >> 1) - 1)
#define STRING_SHORT_MAXCAP         (23)

#define dstring_is_short(str)        ((str)->s.size >> 7)
#define dstring_short_size(str)      ((str)->s.size & 0x7f)

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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dstring.h"

const char *dstring_errname(int err) {
  switch (err) {
  case E_DSTRING_SUCCESS:
    return "Success";
  case E_DSTRING_NGPTR:
    return "Invalid pointer";
  case E_DSTRING_NGVAL:
    return "Invalid value";
  case E_DSTRING_NALLOC:
    return "Allocation failed";
  case E_DSTRING_NGSIZE:
    return "Invalid size";
  case E_DSTRING_OUTRANGE:
    return "Out of range";
  case E_DSTRING_FAIL:
    return "Failed";
  default:
    return "*shrugs*";
  }
}

static inline void dstring_set_short_state(Dstring *str) {
  str->s.size |= 0x80;
}

static inline void dstring_set_long_state(Dstring *str) {
  str->s.size &= 0x7f;
}

static uint64_t ub_power_of_2(uint64_t x) {
  size_t bit_count = 0;
  while (x > 0) {
    x >>= 1;
    bit_count++;
  }
  return 1 << bit_count;
}

static uint64_t next_capacity(uint64_t size) {
  uint64_t capacity = ub_power_of_2(size);
  return capacity < STRING_LONG_MAXCAP ? capacity : STRING_LONG_MAXCAP;
}

static int dstring_grow(Dstring *str, uint64_t length) {
  if (length == 0) {
    return E_DSTRING_SUCCESS;
  }

  uint64_t init_len = dstring_size(str);
  uint64_t reserve = init_len + length + 1;

  if (dstring_is_short(str)) {
    if (reserve > STRING_SHORT_MAXCAP) {
      reserve = next_capacity(reserve);

      char *temp = malloc(reserve);
      if (temp == NULL)
        return E_DSTRING_NALLOC;
      
      memset(temp, 0, reserve);
      memcpy(temp, str->s.data, init_len);

      str->l.data = temp;
      str->l.size = init_len;
      str->l.capacity = reserve;
      dstring_set_long_state(str);
    }
  } else {
    if (reserve > dstring_capacity(str)) {
      reserve = next_capacity(reserve);
      if (reserve >= STRING_LONG_MAXCAP)
        return E_DSTRING_NGSIZE;

      char *temp = realloc(str->l.data, reserve);
      if (temp == NULL)
        return E_DSTRING_NALLOC;

      str->l.data = temp;
      str->l.capacity = reserve;
    }
  }
  return E_DSTRING_SUCCESS;
}

Dstring *dstring_create(void) {
  Dstring *str = malloc(sizeof *str);
  if (str == NULL) {
    dstring_error("Cannot allocate resources for string pointer", E_DSTRING_NALLOC);
    return NULL;
  }

  dstring_init(str);
  return str;
}

Dstring *dstring_from_cstr(const char *src, uint64_t length) {
  Dstring *str = dstring_create();
  if (str == NULL) {
    return NULL;
  }
  dstring_append(str, src, length);
  return str;
}

void dstring_clear(Dstring *str) {
  if (dstring_is_short(str)) {
    memset(str->s.data, 0, STRING_SHORT_MAXCAP);
    str->s.size = 0;
  } else {
    memset(str->l.data, 0, str->l.size);
    str->l.size = 0;
  }
}

void dstring_free(Dstring *str) {
  if (!dstring_is_short(str)) {
    free(str->l.data);
  }
  free(str);
}

void dstring_shrink_opt(Dstring *str) {
  if (!dstring_short_size(str)) {
    uint64_t reserve = next_capacity(str->l.size);
    if (reserve >= str->l.capacity) {
      return;
    }

    char *temp = realloc(str->l.data, reserve);
    if (temp == NULL) {
      dstring_error("Cannot shrink the string", E_DSTRING_NALLOC);
      return;
    }
    str->l.data = temp;
    str->l.capacity = reserve;
  }
}

void dstring_init(Dstring *str) {
  memset(str, 0, sizeof(*str));
  dstring_set_short_state(str);
}

void dstring_assign(Dstring *str, const Dstring *value) {
  Dstring *old = dstring_create();

  memcpy(old, str, sizeof *str);
  memset(str, 0, sizeof *str);

  if (dstring_is_short(value)) {
    memcpy(str->s.data, value->s.data, dstring_short_size(value) + 1);
    str->s.size = dstring_short_size(value);
    dstring_set_short_state(str);
  } else {
    str->l.data = malloc(value->l.capacity);

    if (str->l.data == NULL) {
      dstring_error("Cannot allocate long string buffer to assign string",
        E_DSTRING_NALLOC);
      memcpy(str, old, sizeof *str);
      return;
    }

    memcpy(str->l.data, value->l.data, value->l.size + 1);
    str->l.capacity = value->l.capacity;
    str->l.size = value->l.size;
  }
  dstring_free(old);
}

char dstring_get_at(const Dstring *str, uint64_t pos) {
  if (pos >= dstring_size(str)) {
    dstring_error("Index is equal or larger than string length", E_DSTRING_OUTRANGE);
    return '\0';
  }
  return (dstring_is_short(str) ? str->s.data[pos] : str->l.data[pos]);
}

int dstring_set_at(Dstring *str, uint64_t pos, char value) {
  if (pos >= dstring_size(str)) {
    dstring_error("Index is equal or greater than string length", E_DSTRING_OUTRANGE);
    return E_DSTRING_FAIL;
  } else if (value == '\0') {
    dstring_error("Cannot assign null terminator in the string", E_DSTRING_NGVAL);
    return E_DSTRING_FAIL;
  }

  if (dstring_is_short(str)) {
    str->s.data[pos] = value;
  } else {
    str->l.data[pos] = value;
  }
  return E_DSTRING_SUCCESS;
}

uint64_t dstring_size(const Dstring *str) {
  if (dstring_is_short(str)) {
    return dstring_short_size(str);
  } else {
    return str->l.size;
  }
}

uint64_t dstring_capacity(const Dstring *str) {
  if (dstring_is_short(str)) {
    return STRING_SHORT_MAXCAP;
  } else {
    return str->l.capacity;
  }
}

const char *dstring_to_cstr(const Dstring *str) {
  return (dstring_is_short(str) ? str->s.data : str->l.data);
}

void dstring_insert(Dstring *str, uint64_t pos, const char *value, uint64_t length) {
  if (pos > dstring_size(str)) {
    dstring_error("Index is greater than string length", E_DSTRING_OUTRANGE);
    return;
  } else if (length == 0) {
    return;
  }

  if (dstring_grow(str, length) != E_DSTRING_SUCCESS) {
    dstring_error("Cannot expand capacity to insert string", E_DSTRING_FAIL);
    return;
  }

  if (dstring_is_short(str)) {
    uint8_t init_len = dstring_short_size(str);
    if (pos < init_len) {
      memmove(str->s.data + pos + length, str->s.data + pos, init_len - pos + 1);
    }

    memcpy(str->s.data + pos, value, length + 1);
    str->s.size += length;
  } else {
    memmove(str->l.data + pos + length, str->l.data + pos, str->l.size - pos + 1);
    memcpy(str->l.data + pos, value, length + 1);
    str->l.size += length;
  }
}

void dstring_prepend(Dstring *str, const char *value, uint64_t length) {
  dstring_insert(str, 0, value, length);
}

void dstring_append(Dstring *str, const char *value, uint64_t length) {
  dstring_insert(str, dstring_size(str), value, length);
}

void dstring_concat(Dstring *str, const Dstring *rhs) {
  dstring_append(str, dstring_to_cstr(rhs), dstring_size(rhs));
}

void dstring_erase(Dstring *str, uint64_t pos, uint64_t length) {
  if (pos >= dstring_size(str)) {
    dstring_error("Index is equal or greater than string length", E_DSTRING_OUTRANGE);
    return;
  } else if (length == 0) {
    return;
  }

  uint64_t endpos = pos + length;
  if (endpos > dstring_size(str)) {
    endpos = dstring_size(str);
    length = endpos - pos;
  }

  if (dstring_is_short(str)) {
    uint8_t init_len = dstring_short_size(str);
    memmove(str->s.data + pos, str->s.data + endpos, init_len - endpos + 1);
    str->s.data[init_len - length] = '\0';
    str->s.size -= length;
  } else {
    memmove(str->l.data + pos, str->l.data + endpos, str->l.size - endpos + 1);
    str->l.data[str->l.size - length] = '\0';
    str->l.size -= length;
  }
}

Dstring *dstring_substr(const Dstring *str, uint64_t pos, uint64_t length) {
  if (pos >= dstring_size(str)) {
    dstring_error("Index is greater than string length", E_DSTRING_OUTRANGE);
    return NULL;
  }

  uint64_t diff = dstring_size(str) - pos;
  if (diff < length) {
    length = diff;
  }

  if (dstring_is_short(str)) {
    return dstring_from_cstr(str->s.data + pos, length);
  } else {
    return dstring_from_cstr(str->l.data + pos, length);
  }
}

#ifdef DSTRING_TEST

#include <stdio.h>
#include <string.h>

int test(void) {
  Dstring *a = dstring_create();
  dstring_init(a);

  dstring_append(a, "libido", strlen("libido"));
  dstring_append(a, "than this bruh aa", strlen("than this bruh aa"));
  dstring_append(a, "baby since i left you, i've felt better qiheiheh yeah eh", 56);
  printf("len: %I64d\n%s\n", dstring_size(a), dstring_to_cstr(a));
  dstring_clear(a);
  dstring_append(a, "no way on go", 12);
  printf("%s\n", dstring_to_cstr(a));
  dstring_free(a);

  Dstring *b = dstring_from_cstr("em oi lau dai tinh ai do", 24);
  printf("len: %"PRIu64"\n%s\n", dstring_size(b), dstring_to_cstr(b));


  Dstring c;
  dstring_init(&c);
  dstring_append(&c, "an albino", 9);
  dstring_append(&c, ", a mulatto", 11);
  dstring_append(&c, ", a mosquito", 12);
  dstring_append(&c, ", my libido!", 12);
  printf("%s\n", dstring_to_cstr(&c));
  dstring_clear(&c);
  dstring_append(&c, "hola! vamos!hola! vamos!hola! vamos!", 36);
  printf("%s\n", dstring_to_cstr(&c));
  printf("%"PRIu64"\n", dstring_capacity(&c));
  dstring_concat(&c, b);
  printf("%s\n", dstring_to_cstr(&c));
  printf("%"PRIu64"\n", dstring_capacity(&c));
  printf("%"PRIu64"\n", dstring_size(&c));
  dstring_free(b);

  Dstring *d = dstring_create();
  dstring_append(d, "funko pop post modernism", 24);
  dstring_insert(d, 5, " bababooey ", 11);
  printf("%s\n", dstring_to_cstr(d));
  dstring_clear(d);
  dstring_append(d, "inasama", 7);
  dstring_insert(d, 0, "wenomecha", 9);
  printf("%s\n", dstring_to_cstr(d));
  printf("%"PRIu64"\n", dstring_capacity(d));
  dstring_append(d, " tumajarbisaun", 14);
  printf("%s\n", dstring_to_cstr(d));
  printf("%"PRIu64"\n", dstring_capacity(d));
  dstring_free(d);

  Dstring *e = dstring_from_cstr("amogus sus sussy baka", 21);
  Dstring *f = dstring_from_cstr("mugamuda", 8);
  dstring_assign(e, f);
  printf("%s\n", dstring_to_cstr(e));
  dstring_free(e);
  dstring_free(f);
  e = dstring_from_cstr("they are gonna make me vote for joe biden", 41);
  f = dstring_from_cstr("salute", 6);
  printf("size %"PRIu64"| %s\n", dstring_size(e), dstring_to_cstr(e));
  dstring_assign(e, f);
  printf("size %"PRIu64"| %s\n", dstring_size(e), dstring_to_cstr(e));
  dstring_free(e);
  dstring_free(f);
  e = dstring_from_cstr("they are gonna make me vote for joe biden", 41);
  f = dstring_from_cstr("salute", 6);
  dstring_set_at(e, 5, 'w');
  printf("size %"PRIu64"| %s\n", dstring_size(f), dstring_to_cstr(f));
  dstring_assign(f, e);
  printf("size %"PRIu64"| %s\n", dstring_size(f), dstring_to_cstr(f));
  // dstring_set_at(f, 18, 0);
  dstring_free(e);
  dstring_free(f);

  e = dstring_from_cstr("racist pizza and the quandale dingle", 36);
  f = dstring_from_cstr("kid named finger:", 17);
  dstring_erase(e, 7, 6);
  dstring_erase(f, 10, 28);
  printf("size %"PRIu64"| %s\n", dstring_size(e), dstring_to_cstr(e));
  printf("size %"PRIu64"| %s\n", dstring_size(f), dstring_to_cstr(f));    
  dstring_free(e);
  dstring_free(f);

  Dstring *g = dstring_from_cstr("paranoia agent is a 2004 japanese anime series"
                                 "directed by Kon Satoshi", 69);
  Dstring *h = dstring_substr(g, 58, 15);
  printf("size %"PRIu64"| %s\n", dstring_size(h), dstring_to_cstr(h)); 
  dstring_free(h);
  h = dstring_substr(g, 5, 0);
  printf("size %"PRIu64"| %s\n", dstring_size(h), dstring_to_cstr(h)); 
  dstring_free(h);
  dstring_free(g);

  return 0;
}

#endif /* DSTRING_TEST */

#endif /* DSTRING_H */