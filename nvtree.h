#ifndef NV_TREE_H
#define NV_TREE_H

#include <stddef.h>
#define CVECTOR_LOGARITHMIC_GROWTH
#include "cvector.h"

struct nv_node {
    size_t add_buffer_index;
    size_t length;
    size_t length_left;
};

typedef enum { R, B } nv_colour;

typedef int nv_pool_index;
#define NV_NULL_INDEX -1

#define NODE_FROM_POOL(index) ((struct nv_tree_node*)(index > NV_NULL_INDEX ? nv_pool[index] : NULL))

struct nv_tree_node {
    struct nv_node data;
    nv_pool_index left, right;
    nv_colour colour;
    size_t refcount;
};

extern cvector(struct nv_tree_node*) nv_pool;

nv_pool_index nv_tree_insert(nv_pool_index tree, size_t pos, struct nv_node data);
nv_pool_index nv_tree_paint(nv_pool_index node, nv_colour c);
nv_pool_index nv_tree_init();
void nv_tree_free_all(nv_pool_index tree);
void nv_tree_print(nv_pool_index tree);

#endif
