#ifndef NV_TREE_H
#define NV_TREE_H

#include <stddef.h>
#define CVECTOR_LOGARITHMIC_GROWTH
#include "cvector.h"

#define NV_MAX_BUFFERS 256

typedef enum {
    NV_BUFF_ID_ORIGINAL,
    NV_BUFF_ID_ADD,
    NV_BUFF_ID_DEL,
    NV_BUFF_ID_END,
} nv_buff_id;

extern char* nv_buffers[NV_MAX_BUFFERS];

struct nv_node {
    nv_buff_id buff_id;
    size_t buff_index;
    size_t length;
    size_t length_left;
    size_t lfcount;
};

typedef enum { R, B } nv_colour;

typedef int nv_pool_index;
#define NV_NULL_INDEX -1

struct nv_tree_node {
    struct nv_node data;
    nv_pool_index left, right;
    nv_colour colour;
    size_t refcount;
    size_t length_total;
};

extern cvector(struct nv_tree_node*) nv_pool;

nv_pool_index nv_tree_insert(nv_pool_index tree, size_t pos, struct nv_node data);
nv_pool_index nv_tree_paint(nv_pool_index tree, nv_colour c);
nv_pool_index nv_tree_init();
void nv_tree_free_all(nv_pool_index tree);
void nv_tree_print(nv_pool_index tree);
nv_pool_index nv_find_by_pos(nv_pool_index tree, size_t pos);
nv_pool_index nv_find_by_line(nv_pool_index tree, size_t line);

// TODO: use tree height so there's no guessing
#define NVTREE_MAX_STACK_DEPTH 128
#define NODE_FROM_POOL(index) ((struct nv_tree_node*)(nv_pool && index > NV_NULL_INDEX && index < cvector_size(nv_pool) ? nv_pool[index] : NULL))

#ifdef NVTREE_CONFIG
#include NVTREE_CONFIG
#endif

#ifndef NVTREE_MALLOC
#define NVTREE_MALLOC malloc
#endif

#ifndef NVTREE_FREE
#define NVTREE_FREE free
#endif

#endif
