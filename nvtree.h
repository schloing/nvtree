#ifndef NV_TREE_H
#define NV_TREE_H

struct nv_node {
    size_t index;
    size_t length;
};

struct nv_tree_node;

typedef enum { R, B } nv_colour;

struct nv_tree_node {
    struct nv_node data;
    struct nv_tree_node* left, *right;
    nv_colour colour;
    size_t refcount;
};

#endif
