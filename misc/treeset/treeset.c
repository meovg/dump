#include <string.h>

#include "treeset.h"

RBNode *treeset_new_node(void *value, size_t n) {
  RBNode *node = malloc(sizeof *node + n);
  if (node == NULL) {
    return NULL;
  }

  memcpy(node + 1, value, n);
  return node;
}

RBNode *treeset_find_(RBTree *rbt, void *val, size_t n,
                      int (*compare)(const void *a, const void *b, size_t n))
{
  if (rbt == NULL) {
    return NULL;
  }

  RBNode *cur = rbt->root;

  while (cur != NULL) {
    int dif = compare(val, cur + 1, n);
    if (dif == 0) {
      return cur;
    }
    cur = (dif < 0 ? cur->left : cur->right);
  }
  return NULL;
}

void treeset_insert_(RBTree *rbt, void *val, size_t n,
                     int (*compare)(const void *a, const void *b, size_t n))
{
  if (rbt == NULL) {
    return;
  }

  RBNode **cur = &(rbt->root);
  RBNode *par = NULL;

  while (*cur != NULL) {
    par = *cur;
    int dif = compare(val, *cur + 1, n);
    if (dif == 0) {
      return;
    }
    cur = (dif < 0 ? &((*cur)->left) : &((*cur)->right));
  }

  RBNode *node = treeset_new_node(val, n);
  if (node == NULL) {
    return;
  }
  rb_insert(rbt, node, cur, par);
}