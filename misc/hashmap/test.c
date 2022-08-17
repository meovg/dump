#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "map.h"

typedef MapOf(int) intmap;
typedef MapOf(double) doublemap;

int main(void) {
    intmap *d = malloc(sizeof *d);
    map_init(d);
    map_set(d, "the funny number", 69);
    map_set(d, "the demon number", 666);
    map_set(d, "keyboard mash", 7);
    map_set(d, "adele's age", 34);
    map_set(d, "carlos", 17893);
    printf("%d\n", *map_get(d, "adele's age"));
    printf("%d\n", *map_get(d, "the funny number"));
    map_set(d, "adele's age", 35);
    printf("%d\n", *map_get(d, "adele's age"));
    printf("size of intmap: %"PRIu64"\n", map_size(d));
    map_clear(d);
    free(d);

    doublemap *c = malloc(sizeof *c);
    map_init(c);
    map_set(c, "pi", 3.141592);
    map_set(c, "super idol", 108);
    map_set(c, "my high school score", 27.75);
    printf("size of dmap: %"PRIu64"\n", map_size(c));
    map_set(c, "bread", 18.9222233);
    printf("size of dmap: %"PRIu64"\n", map_size(c));
    printf("%.8f\n", *map_get(c, "pi"));
    map_delete(c, "pi");
    assert(map_get(c, "pi") == NULL);
    printf("size of dmap: %"PRIu64"\n", map_size(c));
    map_clear(c);
    free(c);

    intmap b;
    map_init(&b);
    map_set(&b, "david", 18);
    map_set(&b, "scott bradford", 19);
    map_set(&b, "mooo", 5);
    map_set(&b, "watarinush", 103);
    map_delete(&b, "mooo");
    assert(map_get(&b, "watarinush") != NULL);
    assert(map_get(&b, "david") != NULL);
    map_clear(&b);

    return 0;
}