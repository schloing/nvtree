#include <stdbool.h>
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

    return node;
}

struct nv_tree* nv_tree_init(struct nv_tree* left, struct nv_tree* right, nv_colour c, struct nv_node data)
{
    struct nv_tree* tree = (struct nv_tree*)malloc(sizeof(struct nv_tree));

    if (!tree) {
        return NULL;
    }

    if (left && right) {
        tree->root = nv_tree_node_init(left->root, right->root, c, data);
    }
    else {
        tree->root = nv_tree_node_init(NULL, NULL, c, data);
    }

    return tree;
}

static inline
struct nv_tree* nv_tree_left(struct nv_tree* tree)
{
    if (!tree || !tree->root || !tree->root->left) {
        return NULL;
    }

    return nv_tree_init(NULL, NULL, R, tree->root->left->data);
}

static inline
struct nv_tree* nv_tree_right(struct nv_tree* tree)
{
    if (!tree || !tree->root || !tree->root->right) {
        return NULL;
    }

    return nv_tree_init(NULL, NULL, R, tree->root->right->data);
}

static bool nv_tree_is_doubled_left(struct nv_tree* tree)
{
    struct nv_tree* left = nv_tree_left(tree);

    return tree && tree->root != NULL
        && tree->root->colour == R
        && left
        && left->root != NULL
        && left->root->colour == R;
}

static bool nv_tree_is_doubled_right(struct nv_tree* tree)
{
    struct nv_tree* right = nv_tree_right(tree);

    return tree && tree->root != NULL
        && tree->root->colour == R
        && right
        && right->root != NULL
        && right->root->colour == R;
}

static struct nv_tree* nv_tree_paint(struct nv_tree* tree, nv_colour c)
{
    if (!tree->root) {
        return NULL;
    }

    return nv_tree_init(
        nv_tree_left(tree),
        nv_tree_right(tree),
        c,
        tree->root->data
    );
}

static struct nv_tree* nv_tree_balance(nv_colour c,
        struct nv_tree* left,
        struct nv_tree* right,
        struct nv_node data)
{
    if (c == B && nv_tree_is_doubled_left(left)) {
        return nv_tree_init(
            nv_tree_paint(nv_tree_left(left), B),
            nv_tree_init(nv_tree_right(left), right, B, data),
            R,
            left->root->data
        );
    }
    else if
       (c == B && nv_tree_is_doubled_right(left)) {
        return nv_tree_init(
            nv_tree_init(nv_tree_left(left), nv_tree_right(left), B, left->root->data),
            nv_tree_init(nv_tree_right(nv_tree_right(left)), right, B, data),
            R,
            nv_tree_right(left)->root->data
        );

    }
    else if
       (c == B && nv_tree_is_doubled_left(right)) {
        return nv_tree_init(
            nv_tree_init(left, nv_tree_left(nv_tree_left(right)), B, left->root->data),
            nv_tree_init(nv_tree_right(nv_tree_left(right)), nv_tree_right(right), B, right->root->data),
            R,
            nv_tree_left(right)->root->data
        );
    }
    else if
       (c == B && nv_tree_is_doubled_right(right)) {
        return nv_tree_init(
            nv_tree_init(left, nv_tree_left(right), B, data),
            nv_tree_paint(nv_tree_right(right), B),
            R,
            right->root->data
        );
    }
    else {
        return nv_tree_init(
            left,
            right,
            c,
            data
        );
    }
}

static struct nv_tree* nv_tree_insert(struct nv_tree* tree, struct nv_node data)
{
    if (!tree || !tree->root) {
        return nv_tree_init(NULL, NULL, R, data);
    }

    struct nv_tree* t = (struct nv_tree*)malloc(sizeof(struct nv_tree));
    struct nv_tree_node* root = tree->root;
    nv_colour c = root->colour;

    if (data.index < root->data.index) {
        return
            nv_tree_balance(c, nv_tree_insert(nv_tree_left(tree), data), nv_tree_right(tree), root->data);

//      return
//          nv_tree_init(
//              nv_tree_insert(nv_tree_left(tree), data),
//              nv_tree_right(tree),
//              R,
//              root->data
//          );
    }
    else if
       (root->data.index < data.index) {
        return
            nv_tree_balance(c, nv_tree_left(tree), nv_tree_insert(nv_tree_right(tree), data), root->data);
//      return
//          nv_tree_init(
//              nv_tree_left(tree),
//              nv_tree_insert(nv_tree_right(tree), data),
//              R,
//              root->data
//          );
    }
    else {
        return tree; // no duplicate
    }
}

static void nv_tree_print_inorder(struct nv_tree_node* node)
{
    if (!node) return;

    nv_tree_print_inorder(node->left);
    printf("(%zu,%zu,%c) ", node->data.index, node->data.length, node->colour == R ? 'R' : 'B');
    nv_tree_print_inorder(node->right);
}

static void nv_tree_print(struct nv_tree* tree)
{
    if (!tree || !tree->root) {
        return;
    }

    printf("in order");
    nv_tree_print_inorder(tree->root);
    printf("\n");
}

static struct nv_tree* nv_tree_make_black(struct nv_tree* tree)
{
    if (!tree || !tree->root) {
        return tree;
    }

    struct nv_tree* black = nv_tree_paint(tree, B);
    return black;
}

int main()
{
    struct nv_tree* tree = NULL;
    struct nv_node node;

    size_t values[] = {10, 5, 15, 3, 7, 12, 18};
    size_t n = sizeof(values) / sizeof(values[0]);

    for (size_t i = 0; i < n; i++) {
        node.index = values[i];
        node.length = i + 1;
        printf("%zu...\n", values[i]);

        tree = nv_tree_insert(tree, node);
        tree = nv_tree_make_black(tree);
        nv_tree_print(tree);
    }

    nv_tree_print(tree);

    return 0;
}
