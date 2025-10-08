#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
};

struct nv_tree {
    struct nv_tree_node* root;
};

struct nv_tree_node* nv_tree_node_init(struct nv_tree_node* left, struct nv_tree_node* right, struct nv_node data)
{
    struct nv_tree_node* node = (struct nv_tree_node*)malloc(sizeof(struct nv_tree_node));

    if (!node) {
        return NULL;
    }

    node->left = left;
    node->right = right;
    node->data = data;

    return node;
}

struct nv_tree* nv_tree_init(struct nv_tree* left, struct nv_tree* right, struct nv_node data)
{
    struct nv_tree* tree = (struct nv_tree*)malloc(sizeof(struct nv_tree));

    if (!tree) {
        return NULL;
    }

    if (left && right) {
        tree->root = nv_tree_node_init(left->root, right->root, data);
    }
    else {
        tree->root = nv_tree_node_init(NULL, NULL, data);
    }

    return tree;
}

static inline
struct nv_tree* nv_tree_left(struct nv_tree* tree)
{
    return nv_tree_init(NULL, NULL, tree->root->left->data);
}

static inline
struct nv_tree* nv_tree_right(struct nv_tree* tree)
{
    return nv_tree_init(NULL, NULL, tree->root->right->data);
}

static struct nv_tree* nv_tree_insert(struct nv_tree* tree, struct nv_node data)
{
    if (!tree || !tree->root) {
        return nv_tree_init(NULL, NULL, data);
    }

    struct nv_tree_node* root = tree->root;
    struct nv_tree* t = (struct nv_tree*)malloc(sizeof(struct nv_tree));

    if (data.index < root->data.index) {
        return
            nv_tree_init(
                nv_tree_insert(nv_tree_left(tree), data),
                nv_tree_right(tree),
                root->data
            );
    }
    else if
       (root->data.index < data.index) {
        return
            nv_tree_init(
                nv_tree_left(tree),
                nv_tree_insert(nv_tree_right(tree), data),
                root->data
            );
    }
    else {
        return tree; // no duplicate
    }
}


int main()
{
    return 0;
}
