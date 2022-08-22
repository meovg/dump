#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include "map.h"

typedef MapOf(int) poolmap;

int main(void) {
    poolmap *m = malloc(sizeof *m);
    // clock_t tik = clock();
    map_init(m);

    FILE *f = fopen("words.txt", "r");

    int j = 0;
    char token[7] = {[5] = '\n', [6] = '\0'};
    while (fgets(token, 7, f)) {
        j++;
        token[5] = '\0';
        map_set(m, token, 0);
        // printf("%d", token[5] == '\n');
    }
    fclose(f);
    printf("%d\n", j);

    // for (int i = 0; i < POOL_SIZE; i++) {
        // map_set(m, pool[i], 0);
    // }
    // clock_t tok = clock();
    // float tiam = ((tok - tik) / CLOCKS_PER_SEC) * 1000;
    // printf("time spent: %.9f\n", tiam);
    assert(map_get(m, "bloat") == NULL);
    printf("%"PRIu64"\n", map_size(m));
    map_clear(m);
    free(m);
    return 0;
}