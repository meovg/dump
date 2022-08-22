#ifndef TREESET_H
#define TREESET_H

#include <string.h>

#include "rbtree.h"

typedef RBNode TreeSetNode;

#define TreeSet(T) struct {  \
    RBTree tree; \
    int (*compare)(const void *a, const void *b, size_t n); \
    T val;  \
}

#define treeset_init(set, cmpf) do {  \
    memset((set), 0, sizeof *(set)); \
    (set)->compare = (cmpf) == NULL ? memcmp : (cmpf); \
} while (0)

#define treeset_insert(set, value) do { \
    (set)->val = value;  \
    treeset_insert_(&((set)->tree), &((set)->val), sizeof (set)->val, (set)->compare); \
} while (0)

#define treeset_find(set, value)  \
    ({(set)->val = value;  \
    treeset_find_(&((set)->tree), &((set)->val), sizeof (set)->val, (set)->compare);})

#define treeset_delete(set, value) do { \
    (set)->val = value; \
    RBNode *temp = treeset_find(set, value);  \
    if (temp != NULL) \
    {  \
        rb_delete(&((set)->tree), temp); \
        free(temp); \
    } \
} while (0)


#define treeset_deref(type, node)   (*((type *)((node) + 1)))

#define treeset_size(set)           (rb_size(&((set)->tree)))

#define treeset_clear(set)          (rb_clear(&((set)->tree)))

#define treeset_first(set)          (rb_first(&((set)->tree)))
#define treeset_last(set)           (rb_last(&(set)->tree))

#define treeset_prev(node)          (rb_prev(node))
#define treeset_next(node)          (rb_next(node))

RBNode *treeset_new_node(void *value, size_t n);
RBNode *treeset_find_(RBTree *rbt, void *val, size_t n,
                      int (*compare)(const void *a, const void *b, size_t n));
void treeset_insert_(RBTree *rbt, void *val, size_t n,
                     int (*compare)(const void *a, const void *b, size_t n));

#endif /* TREESET_H */