#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CVECTOR_LOGARITHMIC_GROWTH
#include "cvector.h"
#include "nvtree.h"

static cvector(struct nv_tree_node*) edits;

static struct nv_tree_node* nv_tree_paint(struct nv_tree_node* node, nv_colour c);

static struct nv_tree_node* nv_tree_left(struct nv_tree_node* tree);
static struct nv_tree_node* nv_tree_right(struct nv_tree_node* tree);

static
struct nv_tree_node* nv_tree_make_parent_shared(
    struct nv_tree_node* old,
    struct nv_tree_node* left,
    struct nv_tree_node* right,
    nv_colour colour,
    struct nv_node data
);

static struct nv_tree_node* nv_tree_balance(
    struct nv_tree_node* left,
    struct nv_tree_node* right,
    nv_colour c,
    struct nv_node data
);

static struct nv_tree_node* nv_tree_node_init(
    struct nv_tree_node* left,
    struct nv_tree_node* right,
    nv_colour c,
    struct nv_node data
);

static struct nv_tree_node* nv_tree_insert(struct nv_tree_node* tree, struct nv_node data);

static void nv_tree_print_inorder(struct nv_tree_node* node);
static void nv_tree_print(struct nv_tree_node* tree);
static void nv_tree_free(struct nv_tree_node* tree);

static
struct nv_tree_node* nv_tree_make_parent_shared(
    struct nv_tree_node* old,
    struct nv_tree_node* left,
    struct nv_tree_node* right,
    nv_colour colour,
    struct nv_node data
)
{
    if (old && old->left == left && old->right == right &&
        old->colour == colour &&
        old->data.index == data.index &&
        old->data.length == data.length) {
        return old;  // reuse existing node
    }

    return nv_tree_node_init(left, right, colour, data);
}

static
struct nv_tree_node* nv_tree_node_init(
    struct nv_tree_node* left,
    struct nv_tree_node* right,
    nv_colour c,
    struct nv_node data
)
{
    struct nv_tree_node* node = (struct nv_tree_node*)malloc(sizeof(struct nv_tree_node));

    if (!node) {
        return NULL;
    }

    printf("ALLOCATING %p:%d\n", node, data.index);

    if (left) {
        left->refcount++;
    }

    if (right) {
        right->refcount++;
    }

    node->left = left;
    node->right = right;
    node->data = data;
    node->colour = c;
    node->refcount = 1;

    return node;
}

static struct nv_tree_node* nv_tree_left(struct nv_tree_node* tree)
{
    if (tree && tree->left) {
        return tree->left;
    }

    return NULL;
}

static struct nv_tree_node* nv_tree_right(struct nv_tree_node* tree)
{
    if (tree && tree->right) {
        return tree->right;
    }

    return NULL;
}

static struct nv_tree_node* nv_tree_paint(struct nv_tree_node* node, nv_colour c) {
    if (!node || node->colour == c) {
        return node;
    }

    struct nv_tree_node* result = nv_tree_make_parent_shared(node, node->left, node->right, c, node->data);

    if (result != node) {
        nv_tree_free(node);
    }

    return result;
}

static
struct nv_tree_node* nv_tree_balance(
    struct nv_tree_node* left,
    struct nv_tree_node* right,
    nv_colour c,
    struct nv_node data
)
{
    struct nv_tree_node *l, *r, *result;

    if (c == B && left && left->colour == R && left->left && left->left->colour == R) {
        l = nv_tree_paint(left->left, B);
        r = nv_tree_node_init(left->right, right, B, data);
        result = nv_tree_node_init(l, r, R, left->data);

        nv_tree_free(left);
    }
    else if
       (c == B && left && left->colour == R && left->right && left->right->colour == R) {
        l = nv_tree_node_init(left->left, left->right->left, B, left->data);
        r = nv_tree_node_init(left->right->right, right, B, data);
        result = nv_tree_node_init(l, r, R, left->right->data);
       
        nv_tree_free(left);
    }
    else if
       (c == B && right && right->colour == R && right->left && right->left->colour == R) {
        l = nv_tree_node_init(left, right->left->left, B, data);
        r = nv_tree_node_init(right->left->right, right->right, B, right->data);
        result = nv_tree_node_init(l, r, R, right->left->data);

        nv_tree_free(right);
    }
    else if
       (c == B && right && right->colour == R && right->right && right->right->colour == R) {
        l = nv_tree_node_init(left, right->left, B, data);
        r = nv_tree_paint(right->right, B);
        result =  nv_tree_node_init(l, r, R, right->data);

        nv_tree_free(right);
    }
    else {
        result = nv_tree_node_init(left, right, c, data);
    }

    return result;
}

static struct nv_tree_node* nv_tree_insert(struct nv_tree_node* tree, struct nv_node data)
{
    if (!tree) {
        return nv_tree_node_init(NULL, NULL, R, data);
    }

    struct nv_tree_node *result = tree;

    if (data.index < tree->data.index) {
        result = nv_tree_insert(tree->left, data);
        result = nv_tree_balance(result, tree->right, tree->colour, tree->data);
        nv_tree_free(tree->left);
    }
    else if
       (data.index > tree->data.index) {
        result = nv_tree_insert(tree->right, data);
        result = nv_tree_balance(tree->left, result, tree->colour, tree->data);
        nv_tree_free(tree->right);
    }
    else {
        result->refcount++;
    }

    return result;
}

static void nv_tree_print_inorder(struct nv_tree_node* node)
{
    if (!node) {
        return;
    }

    nv_tree_print_inorder(node->left);
    printf("(%zu,%zu,%c) ", node->data.index, node->data.length, node->colour == R ? 'R' : 'B');
    nv_tree_print_inorder(node->right);
}

static void nv_tree_free(struct nv_tree_node* tree)
{
    if (!tree) {
        return;
    }

    if (--tree->refcount <= 0) {
        printf("FREEING %p: %d\n", tree, (int)tree->data.index);

        nv_tree_free(tree->left);
        nv_tree_free(tree->right);

        tree->left = tree->right = NULL;

        free(tree);
    }
}

static void nv_tree_print(struct nv_tree_node* tree)
{
    if (!tree) {
        return;
    }

    printf("in order");
    nv_tree_print_inorder(tree);
    printf("\n");
}

int main()
{
    struct nv_tree_node* tree = NULL;
    struct nv_node node;

    cvector_reserve(edits, 32);

#define NUM_VALUES 10
    size_t values[NUM_VALUES];

    for (size_t i = 0; i < NUM_VALUES; i++) {
        values[i] = NUM_VALUES - i;

        node.index = values[i];
        node.length = i;

        struct nv_tree_node* new_root = nv_tree_insert(tree, node);

        if (new_root != tree) {
            cvector_push_back(edits, tree);
        }

        tree = nv_tree_paint(new_root, B);
    }

    nv_tree_print(tree);

    for (int i = 0; i < cvector_size(edits); i++) {
        nv_tree_free(edits[i]);
    }

    nv_tree_free(tree);
    cvector_free(edits);

    return 0;
}
