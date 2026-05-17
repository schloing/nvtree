#ifndef NV_TREE_TREE_H
#define NV_TREE_TREE_H

#include <stddef.h>
#include "rb.h"

nv_tree* nv_tree_join(nv_tree_data data, nv_tree* left, nv_tree* right);
size_t nv_tree_size(nv_tree* tree);
void nv_tree_split(nv_tree* tree, size_t offset, nv_tree** left, nv_tree** right);
nv_tree* nv_tree_insert(nv_tree_data data, nv_tree* tree, size_t position);
nv_tree* nv_tree_find_by_offset(nv_tree* tree, size_t offset);
nv_tree* nv_tree_find_by_line(nv_tree* tree, size_t line);
#define NV_TREE_MAX_STACK_DEPTH 128

#endif