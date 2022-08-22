#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "treeset.h"

typedef TreeSet(int) intset;

int intcmpf(const void *a, const void *b, size_t n) {
  const int ai = *(const int *)a;
  const int bi = *(const int *)b;
  if (ai == bi) {
    return 0;
  }
  return ai > bi ? 1 : -1;
}

int main(void) {
  intset a;
  treeset_init(&a, intcmpf);
  clock_t tik = clock();
  for (int i = 0; i < 938332; i++) {
    treeset_insert(&a, i);
  }
  printf("%ld\n", treeset_size(&a));
    // treeset_clear(&a);
  for (int i = 900000; i >= 300000; i--) {
    treeset_delete(&a, i);
  }
  clock_t tok = clock();
  double tmv = (double)((tok -tik) / CLOCKS_PER_SEC);
  printf("%ld\n", treeset_size(&a));
  printf("done in %.9f\n", tmv);
  return 0;
}