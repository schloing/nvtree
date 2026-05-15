#ifndef NV_TREE_H
#define NV_TREE_H

#include <stdbool.h>
#include <stddef.h>

#include "rb_conf.h"
#ifndef NV_TREE_RB_CONF
#error "rb_conf.h must be included by rb.h"
#endif

typedef struct nv_tree_data_s nv_tree_data;

typedef enum nv_tree_colour_e {
    NV_TREE_COLOUR_RED,
    NV_TREE_COLOUR_BLACK,
} nv_tree_colour;

typedef const struct nv_tree_s nv_tree;

struct nv_tree_s {
    nv_tree_colour colour;
    nv_tree *left, *right; // TODO: tag pointer with colour, instead of separate field
    nv_tree_data data;
};

#define NV_TREE_IS_RED(n) ((n) && (n)->colour == NV_TREE_COLOUR_RED)

#ifndef NV_TREE_MALLOC
#define NV_TREE_MALLOC malloc
#endif

#ifndef NV_TREE_FREE
#define NV_TREE_FREE free
#endif

// user defined
bool nv_rb_tree_data_eq(const nv_tree_data* a, const nv_tree_data* b);
bool nv_rb_tree_data_gt(const nv_tree_data* a, const nv_tree_data* b);
bool nv_rb_tree_data_lt(const nv_tree_data* a, const nv_tree_data* b);
nv_tree* nv_rb_tree_recompute(nv_tree* tree);

bool nv_rb_tree_is_member(const nv_tree_data* data, nv_tree* tree);
size_t nv_rb_tree_black_height(nv_tree* tree);
nv_tree* nv_rb_tree_init(nv_tree_colour colour, nv_tree_data data, nv_tree* left, nv_tree* right);
nv_tree* nv_rb_tree_insert_int(nv_tree_data data, nv_tree* tree);
nv_tree* nv_rb_tree_insert(nv_tree_data data, nv_tree* tree);
nv_tree* nv_rb_tree_balance(nv_tree* tree);
nv_tree* nv_rb_tree_make_black(nv_tree* tree);

#define NV_RB_TREE_CHANGE_ARGS nv_tree* tree, nv_tree_colour colour, nv_tree_data data, nv_tree* left, nv_tree* right
nv_tree* nv_rb_tree_change_safe(NV_RB_TREE_CHANGE_ARGS);
nv_tree* nv_rb_tree_change(NV_RB_TREE_CHANGE_ARGS);

#endif