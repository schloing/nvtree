#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "../nvtree.h"
#include "../rb.h"

static void nv_tree_dump_dot_rec(FILE* f, nv_tree* tree)
{
    if (!tree) {
        return;
    }

    fprintf(f,
        "n%p [label=\"%zu (%zu,%zu)\", style=filled, fillcolor=%s, fontcolor=white];\n",
        (void*)tree,
        tree->data.size, tree->data.size_left, (size_t)0,
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

static void nv_tree_dump_dot(nv_tree* tree, const char* path)
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

static size_t nv_tree_validate_sizes(nv_tree* t)
{
    if (!t) {
        return 0;
    }

    size_t L = nv_tree_validate_sizes(t->left);
    size_t computed = L + t->data.size;
    size_t reported = nv_tree_size(t);

    if (reported != computed) {
        fprintf(stderr, "size mismatch at %p: reported=%zu calc=%zu (L=%zu size=%zu R=%zu)\n",
                (void*)t, reported, computed, L, t->data.size, (size_t)0);
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

static void nv_tree_validate(nv_tree* root)
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

#define NV_OUTPUT_DOT_ROOT_PATH "./trees/"

static void nv_generate_dot()
{
    printf("%zu successes\n", success_counter);
    char filename[128];
    snprintf(filename, sizeof(filename), NV_OUTPUT_DOT_ROOT_PATH "tree%zu.dot", success_counter);
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
#define NV_TEST_FIND_BY_OFFSET           2

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
        snprintf(filename, sizeof(filename), NV_OUTPUT_DOT_ROOT_PATH "tree%zu.dot", i + 1);
        nv_tree_dump_dot(tree, filename);

        success_counter++;
    }

    nv_generate_dot();
#elif NV_TEST == NV_TEST_FIND_BY_OFFSET
    tree = nv_tree_insert((nv_tree_data) { .size = 5, .line_count = 1 }, tree, 0); // 0..5
    tree = nv_tree_insert((nv_tree_data) { .size = 5, .line_count = 2 }, tree, 5); // 5..10
    tree = nv_tree_insert((nv_tree_data) { .size = 5, .line_count = 3 }, tree, 10); // 10..15
    nv_tree* new = nv_tree_find_by_offset(tree, 10);
    printf("passed: %d\n", new == tree);
    new = nv_tree_find_by_line(tree, 3);
#endif

    return 0;
}