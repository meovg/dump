#include <string.h>

#include "rbtree.h"

static inline int rb_is_red(const RBNode *node) {
    return node != NULL && node->color == RB_RED;
}

static inline int rb_is_left_child(const RBNode *node) {
    return node->parent->left == node;
}

void rb_init(RBTree *tree) {
    if (tree != NULL) {
        memset(tree, 0, sizeof *tree);
    }
}

static void rb_clear_rec(const RBTree *tree, RBNode *node) {
    if (node != NULL) {
        rb_clear_rec(tree, node->left);
        rb_clear_rec(tree, node->right);
        free(node);
    }
}

void rb_clear(RBTree *tree) {
    if (tree != NULL) {
        rb_clear_rec(tree, tree->root);
        tree->root = NULL;
    }
}

static size_t rb_size_rec(const RBNode *node) {
    if (node == NULL) {
        return 0;
    }
    return rb_size_rec(node->left) + rb_size_rec(node->right) + 1;
}

size_t rb_size(const RBTree *tree) {
    return (tree != NULL ? rb_size_rec(tree->root) : 0);
}

static RBNode *rb_find_leftmost(RBNode *node) {
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}

static RBNode *rb_find_rightmost(RBNode *node) {
    while (node->right != NULL) {
        node = node->right;
    }
    return node;
}

RBNode *rb_first(const RBTree *tree) {
    return tree != NULL ? rb_find_leftmost(tree->root) : NULL;
}

RBNode *rb_last(const RBTree *tree) {
    return tree != NULL ? rb_find_rightmost(tree->root) : NULL;
}

RBNode *rb_prev(const RBNode *node) {
    if (node == NULL) {
        return NULL;
    } else if (node->left != NULL) {
        return rb_find_rightmost(node->left);
    }

    while (node->parent != NULL && rb_is_left_child(node)) {
        node = node->parent;
    }
    return node->parent;
}

RBNode *rb_next(const RBNode *node) {
    if (node == NULL) {
        return NULL;
    } else if (node->right != NULL) {
        return rb_find_leftmost(node->right);
    }

    while (node->parent != NULL && !rb_is_left_child(node)) {
        node = node->parent;
    }
    return node->parent;
}

static void rb_replace_child(RBTree *tree, RBNode *u, RBNode *v) {
    RBNode *parent = u->parent;

    if (parent != NULL) {
        if (parent->left == u)
            parent->left = v;
        else
            parent->right = v;
    } else { 
        tree->root = v;
    }
}

static void rb_left_rotate(RBTree *tree, RBNode *u) {
    RBNode *v = u->right;
    u->right = v->left;
    if (u->right != NULL) {
        u->right->parent = u;
    }

    rb_replace_child(tree, u, v);
    v->parent = u->parent;
    v->left = u;
    u->parent = v;
}

static void rb_right_rotate(RBTree *tree, RBNode *u) {
    RBNode *v = u->left;
    u->left = v->right;
    if (u->left != NULL) {
        u->left->parent = u;
    }

    rb_replace_child(tree, u, v);
    v->parent = u->parent;
    v->right = u;
    u->parent = v;
}

static void rb_insert_repair(RBTree *tree, RBNode *node) {
    RBNode *uncle;
    RBNode *parent;
    RBNode *grandpa; 

    for (;;) {
        parent = node->parent;
        if (!rb_is_red(parent))
            break;

        grandpa = parent->parent;
        if (grandpa == NULL)
            break;

        if (rb_is_left_child(parent)) {
            uncle = grandpa->right;
            if (rb_is_red(uncle)) {
                uncle->color = RB_BLACK;
                parent->color = RB_BLACK;
                grandpa->color = RB_RED;
                node = grandpa;
            } else {
                if (!rb_is_left_child(node)) {
                    node = parent;
                    rb_left_rotate(tree, node);
                    parent = node->parent;
                    grandpa = parent->parent;
                }
                parent->color = RB_BLACK;
                grandpa->color = RB_RED;
                rb_right_rotate(tree, grandpa);
            }
        } else {
            uncle = grandpa->left;
            if (rb_is_red(uncle)) {
                uncle->color = RB_BLACK;
                parent->color = RB_BLACK;
                grandpa->color = RB_RED;
                node = grandpa;
            } else {
                if (rb_is_left_child(node)) {
                    node = parent;
                    rb_right_rotate(tree, node);
                    parent = node->parent;
                    grandpa = parent->parent;
                }
                parent->color = RB_BLACK;
                grandpa->color = RB_RED;
                rb_left_rotate(tree, grandpa);
            }
        }
    }
    tree->root->color = RB_BLACK;
}

void rb_insert(RBTree *tree, RBNode *node, RBNode **dest, RBNode *parent) {
    node->color = RB_RED;
    node->parent = parent;
    node->left = node->right = NULL;

    *dest = node;
    if (parent == NULL) {
        tree->root = node;
    }
    rb_insert_repair(tree, node);
}

static void rb_delete_repair(RBTree *tree, RBNode *node) {
    RBNode *sibling;
    RBNode *parent;

    for (;;) {
        parent = node->parent;
        if (parent == NULL || rb_is_red(node))
            break;

        if (rb_is_left_child(node)) {
            sibling = parent->right;
            if (rb_is_red(sibling)) {
                sibling->color = RB_BLACK;
                parent->color = RB_RED;
                rb_left_rotate(tree, parent);
                parent = node->parent;
                sibling = parent->right;
            }

            if (!rb_is_red(sibling->left) && !rb_is_red(sibling->right)) {
                sibling->color = RB_RED;
                node = parent;
            } else {
                if (!rb_is_red(sibling->right)) {
                    sibling->left->color = RB_BLACK;
                    sibling->color = RB_RED;
                    rb_right_rotate(tree, sibling);
                    parent = node->parent;
                    sibling = parent->right;
                }
                sibling->color = parent->color;
                parent->color = RB_BLACK;
                sibling->right->color = RB_BLACK;
                rb_left_rotate(tree, parent);
                break;
            }
        } else {
            sibling = parent->left;
            if (rb_is_red(sibling)) {
                sibling->color = RB_BLACK;
                parent->color = RB_RED;
                rb_right_rotate(tree, parent);
                parent = node->parent;
                sibling = parent->left;
            }

            if (!rb_is_red(sibling->left) && !rb_is_red(sibling->right)) {
                sibling->color = RB_RED;
                node = parent;
            } else {
                if (!rb_is_red(sibling->left)) {
                    sibling->right->color = RB_BLACK;
                    sibling->color = RB_RED;
                    rb_left_rotate(tree, sibling);
                    parent = node->parent;
                    sibling = parent->left;
                }
                sibling->color = parent->color;
                parent->color = RB_BLACK;
                sibling->left->color = RB_BLACK;
                rb_right_rotate(tree, parent);
                break;
            }
        }
    }
    node->color = RB_BLACK;
}

void rb_delete(RBTree *tree, RBNode *node) {
    RBNode *replacing;

    if (node->left == NULL) {
        if (node->right == NULL) {
            replacing = (rb_is_red(node) ? NULL : node->parent);
        } else {
            node->right->parent = node->parent;
            node->right->color = node->color;
            replacing = NULL;
        }
        rb_replace_child(tree, node, node->right);

    } else if (node->right == NULL) {
        node->left->parent = node->parent;
        node->left->color = node->color;
        replacing = NULL;
        rb_replace_child(tree, node, node->left);

    } else {
        RBNode *next = rb_find_leftmost(node->right);
        RBNode *next_child = next->right;
        RBNode *next_parent = next->parent;
        int deleted_color = next->color;

        if (next != node->right) {
            next_parent->left = next_child;
            if (next_child != NULL) {
                next_child->parent = next_parent;
            }
            node->right->parent = next;
            next->right = node->right;
        }

        node->left->parent = next;
        next->left = node->left;
        rb_replace_child(tree, node, next);
        next->parent = node->parent;
        next->color = node->color;

        if (next_child == NULL) {
            replacing = deleted_color == RB_BLACK ? next_parent : NULL;
        } else {
            next_parent->color = RB_BLACK;
            replacing = NULL; 
        }
    }

    if (replacing != NULL) {
        rb_delete_repair(tree, replacing);
    }
    node->parent = node->left = node->right = NULL;
}