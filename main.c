#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CVECTOR_LOGARITHMIC_GROWTH
#include "cvector.h"
#include "nvtree.h"

cvector(struct nv_tree_node*) nv_pool; // tree node pool

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
        oldn->data.length_left == data.length_left &&
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

nv_pool_index nv_tree_insert(nv_pool_index tree, size_t pos, struct nv_node data)
{
    struct nv_tree_node* node = NODE_FROM_POOL(tree);

    if (!node) {
        data.length_left = pos;
        return nv_tree_node_init(NV_NULL_INDEX, NV_NULL_INDEX, R, data);
    }

    nv_pool_index result = tree;

    size_t length_left = node->data.length_left,
           node_length = node->data.length;

//  printf("\npos: %zu, length: %zu, length_left: %zu, node_length: %zu\n", pos, data.length, length_left, node_length);

    if (pos < length_left + node_length) {
        nv_pool_index new_left = nv_tree_insert(node->left, pos, data);
        result = nv_tree_balance(new_left, node->right, node->colour, node->data);
        NODE_FROM_POOL(result)->data.length_left += data.length;
        nv_tree_free(node->left);
    }
    else if (pos > length_left + node_length) {
        nv_pool_index new_right = nv_tree_insert(node->right, pos - length_left - node_length, data);
        result = nv_tree_balance(node->left, new_right, node->colour, node->data);
        nv_tree_free(node->right);
    }
    else {
        struct nv_tree_node* right = NODE_FROM_POOL(node->right);
        nv_pool_index new_right = nv_tree_node_init(NV_NULL_INDEX, NV_NULL_INDEX, R, data);

        if (right) {
            nv_pool_index inserted = nv_tree_insert(node->right, 0, data);
            result = nv_tree_balance(node->left, inserted, node->colour, node->data);
        } else {
            result = nv_tree_balance(node->left, new_right, node->colour, node->data);
        }
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
    printf("(%zu,%zu,%c) ", n->data.length_left, n->data.length, n->colour == R ? 'R' : 'B');
    nv_tree_print_inorder(n->right);
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

nv_pool_index nv_tree_init()
{
    if (!nv_pool) {
        cvector_reserve(nv_pool, 32);
    }

    return NV_NULL_INDEX;
}

void nv_tree_free_all(nv_pool_index tree)
{
    for (int i = 0; i < cvector_size(nv_pool); i++) {
        nv_tree_free((nv_pool_index)i);
    }

    nv_tree_free(tree);
    cvector_free(nv_pool);
    nv_pool = NULL;
}
