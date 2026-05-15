#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "rb.h"

static void nv_tree_dump_dot_rec(FILE* f, nv_tree* tree)
{
    if (!tree) {
        return;
    }

    fprintf(f,
        "n%p [label=\"%zu (%zu,%zu)\", style=filled, fillcolor=%s, fontcolor=white];\n",
        (void*)tree,
        tree->data.size, tree->data.size_left, tree->data.size_right,
        tree->colour == NV_TREE_COLOUR_RED ? "red" : "black");

    if (tree->left) {
        fprintf(f, "n%p -> n%p;\n", (void*)tree, (void*)tree->left);
        nv_tree_dump_dot_rec(f, tree->left);
    }

    if (tree->right) {
        fprintf(f, "n%p -> n%p;\n", (void*)tree, (void*)tree->right);
        nv_tree_dump_dot_rec(f, tree->right);
    }
}

void nv_tree_dump_dot(nv_tree* tree, const char* path)
{
    FILE* f = fopen(path, "w");

    fprintf(f, "digraph G {\n");
    fprintf(f, "node [shape=circle];\n");
    nv_tree_dump_dot_rec(f, tree);
    fprintf(f, "}\n");

    fclose(f);
}

static nv_tree_data* nv_tree_gimme_data()
{
    static int id = 0;
    nv_tree_data* dat = (nv_tree_data*)malloc(sizeof(nv_tree_data));
    dat->size = 1;
    id++;
    return dat;
}

nv_tree* nv_tree_join_right(nv_tree_data data, nv_tree* left, nv_tree* right)
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
nv_tree* nv_tree_join_left(nv_tree_data data, nv_tree* left, nv_tree* right)
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

    if (offset < tree->data.size_left + tree->data.size) {
        *left = tree->left;
        *right = tree->right;
        return;
    }

    nv_tree *l, *r;
    if (offset < tree->data.size_left) {
        nv_tree_split(tree->left, offset, &l, &r);
        *left = l;
        *right = nv_tree_join(tree->data, r, tree->right);
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
        newdata.size_right = nv_tree_size(right);
        tree = nv_rb_tree_change(tree, tree->colour, newdata, tree->left, right);
    }

    return nv_rb_tree_balance(tree);
}

nv_tree* nv_tree_insert(nv_tree_data data, nv_tree* tree, size_t position)
{
    nv_tree* new = nv_tree_insert_int(data, tree, position);
    return nv_rb_tree_make_black(new);
}

static size_t nv_tree_validate_sizes(nv_tree* t)
{
    if (!t) {
        return 0;
    }

    size_t L = nv_tree_validate_sizes(t->left);
    size_t R = nv_tree_validate_sizes(t->right);
    size_t computed = L + t->data.size + R;
    size_t reported = nv_tree_size(t);

    if (reported != computed) {
        fprintf(stderr, "size mismatch at %p: reported=%zu calc=%zu (L=%zu size=%zu R=%zu)\n",
                (void*)t, reported, computed, L, t->data.size, R);
        exit(0);
    }

    return computed;
}

static int nv_tree_validate_rb(nv_tree* t)
{
    if (!t) {
        return 1;
    }

    if (t->colour == NV_TREE_COLOUR_RED) {
        if (t->left && t->left->colour == NV_TREE_COLOUR_RED) {
            fprintf(stderr, "red-red violation at %p and left %p\n", (void*)t, (void*)t->left);
            exit(0);
        }
        if (t->right && t->right->colour == NV_TREE_COLOUR_RED) {
            fprintf(stderr, "red-red violation at %p and right %p\n", (void*)t, (void*)t->right);
            exit(0);
        }
    }

    int lh = nv_tree_validate_rb(t->left);
    int rh = nv_tree_validate_rb(t->right);

    if (lh != rh) {
        fprintf(stderr, "black-height mismatch at %p: lh=%d rh=%d\n", (void*)t, lh, rh);
        exit(0);
    }

    return lh + (t->colour == NV_TREE_COLOUR_BLACK ? 1 : 0);
}

void nv_tree_validate(nv_tree* root)
{
    nv_tree_validate_sizes(root);
    int bh = nv_tree_validate_rb(root);

    if (root && root->colour != NV_TREE_COLOUR_BLACK) {
        fprintf(stderr, "root not black\n");
        exit(0);
    }

    fprintf(stderr, "validation OK; black-height=%d\n", bh);
}

static nv_tree* tree = NULL;
static size_t success_counter = 0;

#define NV_OUTPUT_DOT_ROOT_PATH "./test/trees/"

void nv_generate_dot()
{
    printf("%zu successes\n", success_counter);
    char filename[128];
    snprintf(filename, sizeof(filename), NV_OUTPUT_DOT_ROOT_PATH "tree%02zu.dot", success_counter);
    nv_tree_dump_dot(tree, filename);
}

ssize_t nv_clamp_min(ssize_t a, ssize_t min)
{
    if (a < min) {
        return min;
    }
    return a;
}

int main()
{
    srand(time(NULL));
    ssize_t position = 0;
    atexit(nv_generate_dot);

#define NV_TEST_CONSECUTIVE_INSERTS      0
#define NV_TEST_CONSECUTIVE_SPLITS       1

#define NV_TEST NV_TEST_CONSECUTIVE_INSERTS

#if NV_TEST == NV_TEST_CONSECUTIVE_INSERTS
    for (int i = 0; i < 100; i++) {
        ssize_t length = i;
        tree = nv_tree_insert((nv_tree_data) { .size = length }, tree, position);
        nv_tree_validate(tree);
        char filename[128];
        snprintf(filename, sizeof(filename), NV_OUTPUT_DOT_ROOT_PATH "tree%d.dot", i);
        nv_tree_dump_dot(tree, filename); // bash script turns *.dot -> *.svg
        position += 1 + i;
        success_counter++;
    }
#elif NV_TEST == NV_TEST_CONSECUTIVE_SPLITS
    tree = nv_tree_insert((nv_tree_data){ .size = 20 }, tree, 0);
    nv_tree_validate(tree);
    nv_tree_dump_dot(tree, NV_OUTPUT_DOT_ROOT_PATH "tree0.dot");
    success_counter++;

    for (size_t i = 0; i < 20; ++i) {
        tree = nv_tree_insert((nv_tree_data){ .size = 1 }, tree, i);
        nv_tree_validate(tree);

        char filename[128];
        snprintf(filename, sizeof(filename), NV_OUTPUT_DOT_ROOT_PATH "tree%02zu.dot", i + 1);
        nv_tree_dump_dot(tree, filename);

        success_counter++;
    }

    nv_generate_dot();
#endif

    return 0;
}