#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "map.h"

typedef MapOf(int) poolmap;

int main(void) {
  poolmap *m = malloc(sizeof *m);
  map_init(m);

  FILE *f = fopen("words.txt", "r");

  int j = 0;
  char token[7] = {[5] = '\n', [6] = '\0'};
  while (fgets(token, 7, f)) {
    j++;
    token[5] = '\0';
    map_set(m, token, 0);
  }
  fclose(f);
  printf("%d\n", j);

  assert(map_get(m, "bloat") != NULL);
  printf("%"PRIu64"\n", map_size(m));
  map_clear(m);
  free(m);
  return 0;
}