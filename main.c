#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CVECTOR_LOGARITHMIC_GROWTH
#include "cvector.h"

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

static cvector(struct nv_tree_node*) edits;

static void nv_tree_free(struct nv_tree_node* tree);

struct nv_tree_node* nv_tree_node_init(struct nv_tree_node* left, struct nv_tree_node* right, nv_colour c, struct nv_node data)
{
    struct nv_tree_node* node = (struct nv_tree_node*)malloc(sizeof(struct nv_tree_node));

    if (!node) {
        return NULL;
    }

    node->left = left;
    node->right = right;
    node->data = data;
    node->colour = c;
    node->refcount = 1;

    return node;
}

static inline
struct nv_tree_node* nv_tree_left(struct nv_tree_node* tree)
{
    if (tree && tree->left) {
        tree->left->refcount++;
        return tree->left;
    }

    return NULL;
}

static inline
struct nv_tree_node* nv_tree_right(struct nv_tree_node* tree)
{
    if (tree && tree->right) {
        tree->right->refcount++;
        return tree->right;
    }

    return NULL;
}

static bool nv_tree_is_doubled_left(struct nv_tree_node* tree)
{
    struct nv_tree_node* left = nv_tree_left(tree);

    return tree
        && tree->colour == R
        && left
        && left->colour == R;
}

static bool nv_tree_is_doubled_right(struct nv_tree_node* tree)
{
    struct nv_tree_node* right = nv_tree_right(tree);

    return tree
        && tree->colour == R
        && right
        && right->colour == R;
}

struct nv_tree_node* nv_tree_paint(struct nv_tree_node* node, nv_colour c) {
    if (!node || node->colour == c) {
        return node;
    }

    return nv_tree_node_init(node->left, node->right, c, node->data);
}

static 
struct nv_tree_node* nv_tree_balance(
    struct nv_tree_node* left,
    struct nv_tree_node* right,
    nv_colour c,
    struct nv_node data
)
{
    struct nv_tree_node* l, *r;

    if (c == B && nv_tree_is_doubled_left(left)) {
        l = nv_tree_paint(left->left, B);
        r = nv_tree_node_init(left->right, right, B, data);

        l->refcount++;
        r->refcount++;

        return nv_tree_node_init(l, r, R, left->data);
    }
    if (c == B && nv_tree_is_doubled_right(left)) {
        l = nv_tree_node_init(left->left, left->right->left, B, left->data);
        r = nv_tree_node_init(left->right->right, right, B, data);

        l->refcount++;
        r->refcount++;
 
        return nv_tree_node_init(l, r, R, left->right->data);
    }
    if (c == B && nv_tree_is_doubled_left(right)) {
        l = nv_tree_node_init(left, right->left->left, B, data);
        r = nv_tree_node_init(right->left->right, right->right, B, right->data);
      
        l->refcount++;
        r->refcount++;

        return nv_tree_node_init(l, r, R, right->left->data);
    }
    if (c == B && nv_tree_is_doubled_right(right)) {
        l = nv_tree_node_init(left, right->left, B, data);
        r = nv_tree_paint(right->right, B);

        l->refcount++;
        r->refcount++;
 
        return nv_tree_node_init(l, r, R, right->data);
    }

    if (left) {
        left->refcount++;
    }

    if (right) {
        right->refcount++;
    }

    return nv_tree_node_init(left, right, c, data);
}

static struct nv_tree_node* nv_tree_insert(struct nv_tree_node* tree, struct nv_node data)
{
    if (!tree) {
        return nv_tree_node_init(NULL, NULL, R, data);
    }

    nv_colour c = tree->colour;

    struct nv_tree_node* l, *r;

    if (data.index < tree->data.index) {
        l = nv_tree_insert(tree->left, data);
        nv_tree_free(tree->left);
        r = nv_tree_balance(l, tree->right, c, tree->data);

        cvector_push_back(edits, r); // save new version
        return r;
    }
    else if
       (tree->data.index < data.index) {
        l = nv_tree_insert(tree->right, data);
        nv_tree_free(tree->right);
        r = nv_tree_balance(tree->left, l, c, tree->data);

        cvector_push_back(edits, r); // save new version
        return r;
    }
    else {
        return tree; // no duplicate
    }
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
        printf("FREEING %p: %d\n", tree, tree->data.index);

        nv_tree_free(tree->left);
        nv_tree_free(tree->right);

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

    size_t values[] = { 2, 1, 3, 4, 5 };
    size_t n = sizeof(values) / sizeof(values[0]);

    for (size_t i = 0; i < n; i++) {
        node.index = values[i];
        node.length = i + 1;

        tree = nv_tree_insert(tree, node);
        tree = nv_tree_paint(tree, B);
//      nv_tree_print(tree);
    }

    nv_tree_print(tree);

    for (int i = 0; i < cvector_size(edits); i++) {
        nv_tree_free(edits[i]);
    }

    cvector_free(edits);

    return 0;
}
