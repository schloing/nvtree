#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CVECTOR_LOGARITHMIC_GROWTH
#include "cvector.h"
#include "nvtree.h"

static cvector(struct nv_tree_node*) nv_pool; // tree node pool
static cvector(struct nv_tree_node*) edits;

static nv_pool_index nv_tree_paint(nv_pool_index node, nv_colour c);
static nv_pool_index nv_tree_left(nv_pool_index tree);
static nv_pool_index nv_tree_right(nv_pool_index tree);

static nv_pool_index nv_tree_make_parent_shared(
    nv_pool_index old,
    nv_pool_index left,
    nv_pool_index right,
    nv_colour colour,
    struct nv_node data
);

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

static nv_pool_index nv_tree_insert(nv_pool_index tree, struct nv_node data);

static void nv_tree_print_inorder(nv_pool_index node);
static void nv_tree_print(nv_pool_index tree);
static void nv_tree_free(nv_pool_index tree);

#define NODE_FROM_POOL(index) ((struct nv_tree_node*)(index > NV_NULL_INDEX ? nv_pool[index] : NULL))

static nv_pool_index nv_tree_make_parent_shared(
    nv_pool_index old,
    nv_pool_index left,
    nv_pool_index right,
    nv_colour colour,
    struct nv_node data
)
{
    struct nv_tree_node *oldn = NODE_FROM_POOL(old),
                        *leftn = NODE_FROM_POOL(left), 
                        *rightn = NODE_FROM_POOL(right);

    if (oldn && oldn->left == left && oldn->right == right &&
        oldn->colour == colour &&
        oldn->data.index == data.index &&
        oldn->data.length == data.length) {
        return old;
    }

    return nv_tree_node_init(left, right, colour, data);
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

static nv_pool_index nv_tree_left(nv_pool_index tree)
{
    struct nv_tree_node* n = NODE_FROM_POOL(tree);
    return (n && n->left) ? n->left : NV_NULL_INDEX;
}

static nv_pool_index nv_tree_right(nv_pool_index tree)
{
    struct nv_tree_node* n = NODE_FROM_POOL(tree);
    return (n && n->right) ? n->right : NV_NULL_INDEX;
}

static nv_pool_index nv_tree_paint(nv_pool_index node, nv_colour c)
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

static nv_pool_index nv_tree_balance(
    nv_pool_index left,
    nv_pool_index right,
    nv_colour c,
    struct nv_node data
)
{
    struct nv_tree_node *ln = NODE_FROM_POOL(left),
                        *rn = NODE_FROM_POOL(right);

    nv_pool_index l = NV_NULL_INDEX, r = NV_NULL_INDEX, result = NV_NULL_INDEX;

    if (c == B && ln &&
            ln->colour == R &&
            NODE_FROM_POOL(ln->left) &&
            NODE_FROM_POOL(ln->left)->colour == R)
    {
        l = nv_tree_paint(ln->left, B);
        r = nv_tree_node_init(ln->right, right, B, data);

        result = nv_tree_node_init(l, r, R, ln->data);
        nv_tree_free(left);
    }
    else if (c == B && ln &&
            ln->colour == R &&
            NODE_FROM_POOL(ln->right) &&
            NODE_FROM_POOL(ln->right)->colour == R)
    {
        l = nv_tree_node_init(ln->left, NODE_FROM_POOL(ln->right)->left, B, ln->data);
        r = nv_tree_node_init(NODE_FROM_POOL(ln->right)->right, right, B, data);

        result = nv_tree_node_init(l, r, R, NODE_FROM_POOL(ln->right)->data);
        nv_tree_free(left);
    }
    else if (c == B && rn &&
            rn->colour == R &&
            NODE_FROM_POOL(rn->left) &&
            NODE_FROM_POOL(NODE_FROM_POOL(rn->left)->left) &&
            NODE_FROM_POOL(NODE_FROM_POOL(rn->left)->left)->colour == R)
    {
        l = nv_tree_node_init(left, NODE_FROM_POOL(rn->left)->left, B, data);
        r = nv_tree_node_init(NODE_FROM_POOL(rn->left)->right, rn->right, B, rn->data);

        result = nv_tree_node_init(l, r, R, NODE_FROM_POOL(rn->left)->data);
        nv_tree_free(right);
    }
    else if (c == B && rn &&
            rn->colour == R &&
            NODE_FROM_POOL(rn->right) &&
            NODE_FROM_POOL(rn->right)->colour == R)
    {
        l = nv_tree_node_init(left, rn->left, B, data);
        r = nv_tree_paint(rn->right, B);

        result = nv_tree_node_init(l, r, R, rn->data);
        nv_tree_free(right);
    }
    else {
        result = nv_tree_node_init(left, right, c, data);
    }

    return result;
}

static nv_pool_index nv_tree_insert(nv_pool_index tree, struct nv_node data)
{
    struct nv_tree_node* n = NODE_FROM_POOL(tree);

    if (!n) {
        return nv_tree_node_init(NV_NULL_INDEX, NV_NULL_INDEX, R, data);
    }

    nv_pool_index result = tree;

    if (data.index < n->data.index) {
        nv_pool_index new_left = nv_tree_insert(n->left, data);

        result = nv_tree_balance(new_left, n->right, n->colour, n->data);
        nv_tree_free(n->left);
    }
    else if (data.index > n->data.index) {
        nv_pool_index new_right = nv_tree_insert(n->right, data);

        result = nv_tree_balance(n->left, new_right, n->colour, n->data);
        nv_tree_free(n->right);
    }
    else {
        n->refcount++;
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
        free(n);
        nv_pool[tree] = NULL;
    }
}

static void nv_tree_print(nv_pool_index tree)
{
    if (!NODE_FROM_POOL(tree)) {
        return;
    }

    printf("in order");
    nv_tree_print_inorder(tree);
    printf("\n");
}

int main()
{
    nv_pool_index tree = NV_NULL_INDEX;
    struct nv_node node;

    cvector_reserve(nv_pool, 32);
    cvector_reserve(edits, 32);

#define NUM_VALUES 500

    for (size_t i = 0; i < NUM_VALUES; i++) {
        node.index = i;
        node.length = i;

        nv_pool_index new_root = nv_tree_insert(tree, node);

        if (new_root != tree) {
            cvector_push_back(edits, NODE_FROM_POOL(tree));
        }

        tree = nv_tree_paint(new_root, B);
    }

    nv_tree_print(tree);

    for (int i = 0; i < cvector_size(nv_pool); i++) {
        nv_tree_free((nv_pool_index)i);
    }

    nv_tree_free(tree);
    cvector_free(nv_pool);
    cvector_free(edits);

    return 0;
}
