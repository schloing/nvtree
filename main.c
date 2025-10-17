#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CVECTOR_LOGARITHMIC_GROWTH
#include "cvector.h"
#include "nvtree.h"

static cvector(struct nv_tree_node*) nv_pool; // tree node pool
static cvector(nv_pool_index) edits;

// forwards
static nv_pool_index nv_tree_make_parent_shared(
    nv_pool_index old,
    nv_pool_index left,
    nv_pool_index right,
    nv_colour colour,
    struct nv_node data
);

static bool nv_tree_is_doubled_left(nv_pool_index tree);
static bool nv_tree_is_doubled_right(nv_pool_index tree);

static nv_pool_index nv_tree_balance(
    nv_pool_index left,
    nv_pool_index right,
    nv_colour c,
    struct nv_node data
);

static nv_pool_index nv_tree_node_init(
    nv_pool_index left,
    nv_pool_index right,
    nv_colour c,
    struct nv_node data
);

static void nv_tree_free(nv_pool_index tree);
static void nv_tree_print_inorder(nv_pool_index node);
// end forwards

#define NODE_FROM_POOL(index) ((struct nv_tree_node*)(index > NV_NULL_INDEX ? nv_pool[index] : NULL))

static nv_pool_index nv_tree_make_parent_shared(
    nv_pool_index old,
    nv_pool_index left,
    nv_pool_index right,
    nv_colour colour,
    struct nv_node data
)
{
    struct nv_tree_node* oldn = NODE_FROM_POOL(old);

    if (oldn && oldn->left == left && oldn->right == right &&
        oldn->colour == colour &&
        oldn->data.index == data.index &&
        oldn->data.length == data.length) {
        return old;
    }

    return nv_tree_node_init(left, right, colour, data);
}

nv_pool_index nv_tree_paint(nv_pool_index node, nv_colour c)
{
    struct nv_tree_node* n = NODE_FROM_POOL(node);

    if (!n || n->colour == c) {
        return node;
    }

    nv_pool_index result = nv_tree_make_parent_shared(node, n->left, n->right, c, n->data);

    if (result != node) {
        nv_tree_free(node);
    }

    return result;
}

static bool nv_tree_is_doubled_left(nv_pool_index tree)
{
    struct nv_tree_node* node = NODE_FROM_POOL(tree);

    if (!node) {
        return false;
    }

    struct nv_tree_node* leftn = NODE_FROM_POOL(node->left);
    struct nv_tree_node* leftleftn = leftn ? NODE_FROM_POOL(leftn->left) : NULL;

    return node->colour == R
           && leftleftn
           && leftleftn->colour == R;
}

static bool nv_tree_is_doubled_right(nv_pool_index tree)
{
    struct nv_tree_node* node = NODE_FROM_POOL(tree);

    if (!node) {
        return false;
    }

    struct nv_tree_node* rightn = NODE_FROM_POOL(node->right);
    struct nv_tree_node* rightrightn = rightn ? NODE_FROM_POOL(rightn->right) : NULL;

    return node->colour == R
           && rightrightn
           && rightrightn->colour == R;
}

static nv_pool_index nv_tree_node_init(
    nv_pool_index left,
    nv_pool_index right,
    nv_colour c,
    struct nv_node data
)
{
    struct nv_tree_node* node = (struct nv_tree_node*)malloc(sizeof(struct nv_tree_node));

    if (!node) {
        return NV_NULL_INDEX;
    }

    cvector_push_back(nv_pool, node);

    struct nv_tree_node *leftn = NODE_FROM_POOL(left),
                        *rightn = NODE_FROM_POOL(right);

    if (leftn) {
        leftn->refcount++;
    }

    if (rightn) {
        rightn->refcount++;
    }

    node->left = left;
    node->right = right;
    node->data = data;
    node->colour = c;
    node->refcount = 1;

    return (nv_pool_index)(cvector_size(nv_pool) - 1);
}

static nv_pool_index nv_tree_balance(
    nv_pool_index left,
    nv_pool_index right,
    nv_colour c,
    struct nv_node data
)
{
    struct nv_tree_node *leftn = NODE_FROM_POOL(left),
                        *rightn = NODE_FROM_POOL(right);

    nv_pool_index l = NV_NULL_INDEX, r = NV_NULL_INDEX, result = NV_NULL_INDEX;

    if (c == B && nv_tree_is_doubled_left(left)) {
        l = nv_tree_paint(leftn->left, B);
        r = nv_tree_node_init(leftn->right, right, B, data);
        result = nv_tree_node_init(l, r, R, leftn->data);

        nv_tree_free(left);
    }
    else if (c == B && nv_tree_is_doubled_right(left)) {
        l = nv_tree_node_init(leftn->left, NODE_FROM_POOL(leftn->right)->left, B, leftn->data);
        r = nv_tree_node_init(NODE_FROM_POOL(leftn->right)->right, right, B, data);
        result = nv_tree_node_init(l, r, R, NODE_FROM_POOL(leftn->right)->data);

        nv_tree_free(left);
    }
    else if (c == B && nv_tree_is_doubled_left(right)) {
        l = nv_tree_node_init(left, NODE_FROM_POOL(rightn->left)->left, B, data);
        r = nv_tree_node_init(NODE_FROM_POOL(rightn->left)->right, rightn->right, B, rightn->data);
        result = nv_tree_node_init(l, r, R, NODE_FROM_POOL(rightn->left)->data);

        nv_tree_free(right);
    }
    else if (c == B && nv_tree_is_doubled_right(right)) {
        l = nv_tree_node_init(left, rightn->left, B, data);
        r = nv_tree_paint(rightn->right, B);
        result = nv_tree_node_init(l, r, R, rightn->data);

        nv_tree_free(right);
    }
    else {
        result = nv_tree_node_init(left, right, c, data);
    }

    return result;
}

nv_pool_index nv_tree_insert(nv_pool_index tree, struct nv_node data)
{
    struct nv_tree_node* node = NODE_FROM_POOL(tree);

    if (!node) {
        return nv_tree_node_init(NV_NULL_INDEX, NV_NULL_INDEX, R, data);
    }

    nv_pool_index result = tree;

    if (data.index < node->data.index) {
        nv_pool_index new_left = nv_tree_insert(node->left, data);
        result = nv_tree_balance(new_left, node->right, node->colour, node->data);
        nv_tree_free(node->left);
    }
    else if (data.index > node->data.index) {
        nv_pool_index new_right = nv_tree_insert(node->right, data);
        result = nv_tree_balance(node->left, new_right, node->colour, node->data);
        nv_tree_free(node->right);
    }
    else {
        node->refcount++;
    }

    return result;
}

static void nv_tree_print_inorder(nv_pool_index node)
{
    struct nv_tree_node* n = NODE_FROM_POOL(node);

    if (!n) {
        return;
    }

    nv_tree_print_inorder(n->left);
    printf("(%zu,%zu,%c) ", n->data.index, n->data.length, n->colour == R ? 'R' : 'B');
    nv_tree_print_inorder(n->right);
}

static void nv_tree_free(nv_pool_index tree)
{
    struct nv_tree_node* n = NODE_FROM_POOL(tree);

    if (!n) {
        return;
    }

    if (--n->refcount <= 0) {
        nv_tree_free(n->left);
        nv_tree_free(n->right);
        n->left = n->right = NV_NULL_INDEX;
        nv_pool[tree] = NULL;
        free(n);
    }
}

void nv_tree_print(nv_pool_index tree)
{
    if (!NODE_FROM_POOL(tree)) {
        return;
    }

    printf("in order");
    nv_tree_print_inorder(tree);
    printf("\n");
}

nv_pool_index nv_tree_init()
{
    cvector_reserve(nv_pool, 32);
    cvector_reserve(edits, 32);
    return NV_NULL_INDEX;
}

void nv_tree_free_all(nv_pool_index tree)
{
    for (int i = 0; i < cvector_size(nv_pool); i++) {
        nv_tree_free((nv_pool_index)i);
    }

    nv_tree_free(tree);
    cvector_free(nv_pool);
    cvector_free(edits);
}
