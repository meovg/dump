#include <stdio.h>
#include <string.h>

#include "dstring.h"

int main(void) {
  Dstring *a = dstring_create();
  dstring_init(a);

  dstring_append(a, "libido", strlen("libido"));
  dstring_append(a, "than this bruh aa", strlen("than this bruh aa"));
  dstring_append(a, "baby since i left you, i've felt better qiheiheh yeah eh", 56);
  printf("len: %"PRIu64"\n%s\n", dstring_size(a), dstring_to_cstr(a));
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