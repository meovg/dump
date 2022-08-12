#include <stdio.h>
#include <stdlib.h>

#include "treeset.h"

typedef TreeSet(int) intset;

int intcmpf(const void *a, const void *b, size_t n) {
    const int ai = *(const int *)a;
    const int bi = *(const int *)b;
    if (ai == bi)
        return 0;
    return ai > bi ? 1 : -1;
}

int main(void) {
    intset a;
    treeset_init(&a, intcmpf);
    for (int i = 0; i < 938332; i++) {
        treeset_insert(&a, i);
    }
    printf("%d\n", treeset_size(&a));
    // treeset_clear(&a);
    for (int i = 900000; i >= 300000; i--) {
        treeset_delete(&a, i);
    }
    printf("%d\n", treeset_size(&a));
    return 0;
}