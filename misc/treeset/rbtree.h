#ifndef RBTREE_H
#define RBTREE_H

#include <stdlib.h>

#define RB_BLACK    (0)
#define RB_RED      (1)

typedef struct RBNode RBNode;

struct RBNode {
    int color;
    RBNode *left, *right, *parent;
};

typedef struct RBTree RBTree;

struct RBTree {
    RBNode *root;
};

void rb_clear(RBTree *tree);

size_t rb_size(const RBTree *tree);

RBNode *rb_first(const RBTree *tree);

RBNode *rb_last(const RBTree *tree);

RBNode *rb_prev(const RBNode *node);

RBNode *rb_next(const RBNode *node);

void rb_insert(RBTree *tree, RBNode *node, RBNode **dest, RBNode *parent);

void rb_delete(RBTree *tree, RBNode *node);

#endif /* RBTREE_H */