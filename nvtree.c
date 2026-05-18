#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "rb.h"
#include "nvtree.h"

static nv_tree* nv_tree_join_right(nv_tree_data data, nv_tree* left, nv_tree* right);
static nv_tree* nv_tree_join_left(nv_tree_data data, nv_tree* left, nv_tree* right);
static nv_tree* nv_tree_insert_int(nv_tree_data data, nv_tree* tree, size_t offset);

nv_tree* nv_tree_init()
{
    return nv_rb_tree_init(NV_TREE_COLOUR_BLACK, (nv_tree_data) { 0 }, NULL, NULL);
}

bool nv_rb_tree_data_eq(const nv_tree_data* a, const nv_tree_data* b)
{
    return false;
}

bool nv_rb_tree_data_lt(const nv_tree_data* a, const nv_tree_data* b)
{
    return false;
}

bool nv_rb_tree_data_gt(const nv_tree_data* a, const nv_tree_data* b)
{
    return false;
}

size_t nv_tree_size(nv_tree* tree)
{
    return tree ?
        tree->data.size_left + tree->data.size : 0;
}

nv_tree* nv_rb_tree_recompute(nv_tree* tree)
{
    if (!tree) {
        return NULL;
    }

    nv_tree_data data = tree->data;
    data.size_left = nv_tree_size(tree->left);

    return nv_rb_tree_change_safe(tree, tree->colour, data, tree->left, tree->right);
}

static nv_tree* nv_tree_join_right(nv_tree_data data, nv_tree* left, nv_tree* right)
{
    if (!left || !right) {
        return NULL;
    }

    size_t left_bh = nv_rb_tree_black_height(left), right_bh = nv_rb_tree_black_height(right);
    if (left->colour == NV_TREE_COLOUR_BLACK && left_bh == right_bh) {
        return nv_rb_tree_init(NV_TREE_COLOUR_RED, data, left, right);
    }

    // T'=Node(TL.left,⟨TL.key,TL.color⟩,joinRightRB(TL.right,k,TR))
    nv_tree* new = nv_rb_tree_init(
        left->colour,
        left->data,
        left,
        nv_tree_join_right(data, left->right, right)
    );

    if (left->colour == NV_TREE_COLOUR_BLACK
        && NV_TREE_IS_RED(new->right)
        && NV_TREE_IS_RED(new->right->right)) {
        // T'.right.right.color=black;
        new = nv_rb_tree_change(
            new->right->right,
            NV_TREE_COLOUR_BLACK,
            new->right->right->data,
            new->right->right->left,
            new->right->right->right
        );
        return nv_rb_tree_balance(new);
    }

    return new;
}

// symmetric to nv_tree_join_right
static nv_tree* nv_tree_join_left(nv_tree_data data, nv_tree* left, nv_tree* right)
{
    if (!left || !right) {
        return NULL;
    }

    size_t left_bh = nv_rb_tree_black_height(left), right_bh = nv_rb_tree_black_height(right);
    if (right->colour == NV_TREE_COLOUR_BLACK && right_bh == left_bh) {
        return nv_rb_tree_init(NV_TREE_COLOUR_RED, data, left, right);
    }

    nv_tree* new = nv_rb_tree_init(
        right->colour,
        right->data,
        nv_tree_join_left(data, left, right->left),
        right
    );

    if (right->colour == NV_TREE_COLOUR_BLACK
        && NV_TREE_IS_RED(new->left)
        && NV_TREE_IS_RED(new->left->left)) {
        new = nv_rb_tree_change(
            new->left->left,
            NV_TREE_COLOUR_BLACK,
            new->left->left->data,
            new->left->left->left,
            new->left->left->right
        );
        return nv_rb_tree_balance(new);
    }

    return new;
}

// Join
// https://en.wikipedia.org/wiki/Red%E2%80%93black_tree#:~:text=Join%3A%20The,trees,-%2E
nv_tree* nv_tree_join(nv_tree_data data, nv_tree* left, nv_tree* right)
{
    if (!left || !right) {
        return NULL;
    }

    size_t left_bh = nv_rb_tree_black_height(left), right_bh = nv_rb_tree_black_height(right);

    if (left_bh > right_bh) {
        nv_tree* new = nv_tree_join_right(data, left, right);
        if (NV_TREE_IS_RED(new) && NV_TREE_IS_RED(new->right)) {
            new = nv_rb_tree_change(new, NV_TREE_COLOUR_BLACK, new->data, new->left, new->right);
        }
        return new;
    }

    if (right_bh > left_bh) {
        nv_tree* new = nv_tree_join_left(data, left, right);
        if (NV_TREE_IS_RED(new) && NV_TREE_IS_RED(new->left)) {
            new = nv_rb_tree_change(new, NV_TREE_COLOUR_BLACK, new->data, new->left, new->right);
        }
        return new;
    }

    if (left->colour == NV_TREE_COLOUR_BLACK && right->colour == NV_TREE_COLOUR_BLACK) {
        return nv_rb_tree_init(NV_TREE_COLOUR_RED, data, left, right);
    }

    return nv_rb_tree_init(NV_TREE_COLOUR_BLACK, data, left, right);
}

void nv_tree_split(nv_tree* tree, size_t offset, nv_tree** left, nv_tree** right)
{
    if (!tree) {
        *left = NULL;
        *right = NULL;
        return;
    }

    nv_tree *l, *r;
    if (offset < tree->data.size_left) {
        nv_tree_split(tree->left, offset, &l, &r);
        *left = l;
        *right = nv_tree_join(tree->data, r, tree->right);
        return;
    }

    if (offset < tree->data.size_left + tree->data.size) {
        if (tree->left && tree->right) {
            *left = tree->left;
            *right = tree->right;
        }
        else {
            // construct both sides ourselves
            l = nv_rb_tree_init(NV_TREE_COLOUR_BLACK, (nv_tree_data) { .size = offset + 1 }, NULL, NULL);
            r = nv_rb_tree_init(NV_TREE_COLOUR_BLACK, (nv_tree_data) { .size = tree->data.size - offset - 1 }, NULL, NULL);
            *left = l;
            *right = r;
        }
        return;
    }

    nv_tree_split(tree->right, offset, &l, &r);
    *left = nv_tree_join(tree->data, tree->left, l);
    *right = r;
}

static nv_tree* nv_tree_insert_int(nv_tree_data data, nv_tree* tree, size_t offset)
{
    if (!tree) {
        return nv_rb_tree_init(NV_TREE_COLOUR_RED, data, NULL, NULL);
    }

    if (offset < tree->data.size_left) {
        // continue into left subtree
        nv_tree* left = nv_tree_insert_int(data, tree->left, offset);
        nv_tree_data newdata = tree->data;
        newdata.size_left = nv_tree_size(left);
        tree = nv_rb_tree_change(tree, tree->colour, newdata, left, tree->right);
    }
    else if (offset < tree->data.size_left + tree->data.size) {
        size_t rel = offset - tree->data.size_left;
        nv_tree *left, *right;
        nv_tree_split(tree, rel, &left, &right);
        tree = nv_tree_join(data, left, right);
    }
    else {
        // continue into right subtree
        nv_tree* right = nv_tree_insert_int(data, tree->right, offset - (tree->data.size_left + tree->data.size));
        nv_tree_data newdata = tree->data;
        tree = nv_rb_tree_change(tree, tree->colour, newdata, tree->left, right);
    }

    return nv_rb_tree_balance(tree);
}

nv_tree* nv_tree_insert(nv_tree_data data, nv_tree* tree, size_t position)
{
    nv_tree* new = nv_tree_insert_int(data, tree, position);
    return nv_rb_tree_make_black(new);
}

// TODO: optionally return stack in nv_tree_find_* so that
// it can be cached by caller
nv_tree* nv_tree_find_by_offset(nv_tree* tree, size_t offset)
{
    if (!tree) {
        return NULL;
    }

    nv_tree* stack[NV_TREE_MAX_STACK_DEPTH];
    size_t top = 0;
    stack[top++] = tree;

    while (top > 0) {
        nv_tree* curr = stack[--top];
        if (!curr) {
            continue;
        }

        if (curr->data.size_left > offset) {
            if (top < NV_TREE_MAX_STACK_DEPTH) {
                stack[top++] = curr->left;
            }
        }
        else if (curr->data.size_left + curr->data.size >= offset) {
            // requested offset is within current node
            // caller needs to extract it themselves
            return curr;
        }
        else {
            if (top < NV_TREE_MAX_STACK_DEPTH) {
                stack[top++] = curr->right;
            }
        }
    }
    return NULL;
}

nv_tree* nv_tree_find_by_line(nv_tree* tree, size_t line)
{
    if (!tree) {
        return NULL;
    }

    nv_tree* stack[NV_TREE_MAX_STACK_DEPTH];
    size_t top = 0, cum = 0;
    stack[top++] = tree;

    while (top > 0) {
        nv_tree* curr = stack[--top];
        if (!curr) {
            continue;
        }

        if (curr->data.line_count + cum > line) {
            if (top < NV_TREE_MAX_STACK_DEPTH) {
                line -= curr->data.line_count;
                stack[top++] = curr->left;
            }
        }
        else if (curr->data.line_count + curr->data.line_count + cum >= line) {
            // requested line is within current node
            // caller needs to extract it themselves
            return curr;
        }
        else {
            if (top < NV_TREE_MAX_STACK_DEPTH) {
                cum += curr->data.line_count;
                stack[top++] = curr->right;
            }
        }
    }
    return NULL;
}