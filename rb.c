#include <stdlib.h>
#include <stdbool.h>
#include "rb.h"

nv_tree* nv_rb_tree_insert_int(nv_tree_data data, nv_tree* tree);

nv_tree* nv_rb_tree_init(nv_tree_colour colour, nv_tree_data data, nv_tree* left, nv_tree* right)
{
    struct nv_tree_s* tree = NV_TREE_MALLOC(sizeof(struct nv_tree_s));
    if (!tree) {
        return NULL;
    }
    tree->left = left;
    tree->right = right;
    tree->data = data;
    tree->colour = colour;
    return (nv_tree*)tree;
}

// logn search for data from root
bool nv_rb_tree_is_member(const nv_tree_data* data, nv_tree* tree)
{
    if (!tree) {
        return false;
    }

    if (nv_rb_tree_data_lt(data, &tree->data)) {
        return nv_rb_tree_is_member(data, tree->left);
    }
    if (nv_rb_tree_data_eq(data, &tree->data)) {
        return true;
    }
    if (nv_rb_tree_data_gt(data, &tree->data)) {
        return nv_rb_tree_is_member(data, tree->right);
    }

    return false;
}

// does not allocate new node if tree is NULL
nv_tree* nv_rb_tree_make_black(nv_tree* tree)
{
    if (!tree || tree->colour == NV_TREE_COLOUR_BLACK) {
        return tree;
    }

    return nv_rb_tree_init(NV_TREE_COLOUR_BLACK, tree->data, tree->left, tree->right);
}

size_t nv_rb_tree_black_height(nv_tree* tree)
{
    size_t bh = 0;
    while (tree) {
        if (tree->colour == NV_TREE_COLOUR_BLACK) {
            bh++;
        }
        tree = tree->left;
    }

    return bh;
}

// zero chance this shit is correct
nv_tree* nv_rb_tree_balance(nv_tree* tree)
{
    if (!tree) {
        return NULL;
    }
    // if (tree->colour != NV_TREE_COLOUR_BLACK) {
    //     return tree;
    // }

    nv_tree *left = tree->left, *right = tree->right;
    if (!left && !right) {
        return tree;
    }

    if (NV_TREE_IS_RED(left) && NV_TREE_IS_RED(left->left)) {
        return nv_rb_tree_change(left,
                    NV_TREE_COLOUR_RED,
                    left->data,
                    nv_rb_tree_make_black(left->left),
                    nv_rb_tree_change(tree,
                        NV_TREE_COLOUR_BLACK,
                        tree->data,
                        nv_rb_tree_make_black(left->right),
                        nv_rb_tree_make_black(right)
                    )
                );
    }

    if (NV_TREE_IS_RED(left) && NV_TREE_IS_RED(left->right)) {
        return nv_rb_tree_change(left->right,
                    NV_TREE_COLOUR_RED,
                    left->right->data,
                    nv_rb_tree_change(left,
                        NV_TREE_COLOUR_BLACK,
                        left->data,
                        nv_rb_tree_make_black(left->left),
                        nv_rb_tree_make_black(left->right->left)
                    ),
                    nv_rb_tree_change(tree,
                        NV_TREE_COLOUR_BLACK,
                        tree->data,
                        nv_rb_tree_make_black(left->right->right),
                        nv_rb_tree_make_black(right)
                    )
                );
    }

    if (NV_TREE_IS_RED(right) && NV_TREE_IS_RED(right->left)) {
        return nv_rb_tree_change(right->left,
                    NV_TREE_COLOUR_RED,
                    right->left->data,
                    nv_rb_tree_change(tree,
                        NV_TREE_COLOUR_BLACK,
                        tree->data,
                        nv_rb_tree_make_black(left),
                        nv_rb_tree_make_black(right->left->left)
                    ),
                    nv_rb_tree_change(right,
                        NV_TREE_COLOUR_BLACK,
                        right->data,
                        nv_rb_tree_make_black(right->left->right),
                        nv_rb_tree_make_black(right->right)
                    )
                );
    }

    if (NV_TREE_IS_RED(right) && NV_TREE_IS_RED(right->right)) {
        return nv_rb_tree_change(right,
                    NV_TREE_COLOUR_RED,
                    right->data,
                    nv_rb_tree_change(tree,
                        NV_TREE_COLOUR_BLACK,
                        tree->data,
                        nv_rb_tree_make_black(left),
                        nv_rb_tree_make_black(right->left)
                    ),
                    nv_rb_tree_make_black(right->right)
                );
    }

    return nv_rb_tree_recompute(tree);
}

// does not mutate the tree
// structurally shares when possible
// does not trigger recompute
nv_tree* nv_rb_tree_change_safe(nv_tree* tree, nv_tree_colour colour, nv_tree_data data, nv_tree* left, nv_tree* right)
{
    if (!tree) {
        return nv_rb_tree_init(colour, data, left, right);
    }

    if (nv_rb_tree_data_eq(&tree->data, &data) && tree->colour == colour && tree->left == left && tree->right == right) {
        return tree;
    }

    return nv_rb_tree_init(colour, data, left, right);
}

// wrapper nv_rb_tree_change_safe
// will trigger recompute
nv_tree* nv_rb_tree_change(nv_tree* tree, nv_tree_colour colour, nv_tree_data data, nv_tree* left, nv_tree* right)
{
    nv_tree* new = nv_rb_tree_change_safe(tree, colour, data, left, right);
    return nv_rb_tree_recompute(new);
}

nv_tree* nv_rb_tree_insert_int(nv_tree_data data, nv_tree* tree)
{
    if (!tree) {
        return nv_rb_tree_init(NV_TREE_COLOUR_RED, data, NULL, NULL);
    }

    if (nv_rb_tree_data_lt(&data, &tree->data)) {
        tree = nv_rb_tree_balance(nv_rb_tree_change(tree, tree->colour, tree->data, nv_rb_tree_insert_int(data, tree->left), tree->right));
    }
    else if (nv_rb_tree_data_gt(&data, &tree->data)) {
        tree = nv_rb_tree_balance(nv_rb_tree_change(tree, tree->colour, tree->data, tree->left, nv_rb_tree_insert_int(data, tree->right)));
    }

    return tree;
}

nv_tree* nv_rb_tree_insert(nv_tree_data data, nv_tree* tree)
{
    nv_tree* new = nv_rb_tree_insert_int(data, tree);
    return nv_rb_tree_make_black(new);
}