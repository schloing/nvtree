#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../nvtree.h"

#define ADD_BUFFER_SIZE 256
static cvector(char) add_buffer;

// forwards
static void assign_insert(nv_pool_index* tree, const char* string, size_t pos);
static void assign_insert_print(nv_pool_index* tree, const char* string, size_t pos);
static void print(nv_pool_index tree);
static int test_try_right_insertions();
static int test_try_left_insertions();
static int test_try_contiguous_insertions();
static int test_middle_insertions();
// end forwards

static void assign_insert(nv_pool_index* tree, const char* string, size_t pos)
{
    size_t push_index = cvector_size(add_buffer), i = 0;

    for (i = 0; string[i] != '\0'; i++) {
        cvector_push_back(add_buffer, string[i]);
    }

    struct nv_node data = { .add_buffer_index = push_index, .length = i, .length_left = 0 };

    *tree = nv_tree_insert(*tree, pos, data);
    *tree = nv_tree_paint(*tree, B);
}

static void assign_insert_print(nv_pool_index* tree, const char* string, size_t pos)
{
    assign_insert(tree, string, pos);
    printf("\n");
    print(*tree);
}

static void print(nv_pool_index node)
{
    struct nv_tree_node* n = NODE_FROM_POOL(node);

    if (!n) {
        return;
    }

    print(n->left);

    size_t bufsize = cvector_size(add_buffer);
    if (n->data.add_buffer_index < bufsize &&
        n->data.add_buffer_index + n->data.length <= bufsize) {
        printf("%.*s\n", (int)n->data.length, &add_buffer[n->data.add_buffer_index]);
    }

    print(n->right);
}

static int test_try_right_insertions()
{
    printf("attempting right insertions\n------------------------------\n");

    nv_pool_index tree = nv_tree_init();

    size_t pos = 0;
    assign_insert_print(&tree, "monday", pos); pos += strlen("monday");
    assign_insert_print(&tree, "tuesday", pos); pos += strlen("tuesday");
    assign_insert_print(&tree, "wednesday", pos);

    nv_tree_free_all(tree);

    return 1;
}

static int test_try_left_insertions()
{
    printf("attempting left insertions\n------------------------------\n");

    nv_pool_index tree = nv_tree_init();

    assign_insert_print(&tree, "wednesday", 0);
    assign_insert_print(&tree, "tuesday", 0);
    assign_insert_print(&tree, "monday", 0);

    nv_tree_free_all(tree);

    return 1;
}

static int test_try_contiguous_insertions()
{
    printf("attempting contiguous insertions\n------------------------------\n");

    nv_pool_index tree = nv_tree_init();
    size_t pos = 0;

    assign_insert_print(&tree, "monday", pos);      pos += strlen("monday");
    assign_insert_print(&tree, "tuesday", pos);     pos += strlen("tuesday");
    assign_insert_print(&tree, "wednesday", pos);   pos += strlen("wednesday");
    assign_insert_print(&tree, "thursday", pos);    pos += strlen("thursday");
    assign_insert_print(&tree, "friday", pos);      pos += strlen("friday");
    assign_insert_print(&tree, "saturday", pos);

    nv_tree_free_all(tree);

    return 1;
}

static int test_middle_insertions()
{
    printf("attempting middle insertions\n------------------------------\n");

    nv_pool_index tree = nv_tree_init();

    assign_insert_print(&tree, "mnday", 0);
    assign_insert_print(&tree, "o", 1);

    nv_tree_free_all(tree);

    return 1;
}

// TODO: automate verification of trees, printing and manually checking isn't a good test
int main()
{
    cvector_reserve(add_buffer, ADD_BUFFER_SIZE);

    assert(test_try_right_insertions());
    printf("\n");
    assert(test_try_left_insertions());
    printf("\n");
    assert(test_try_contiguous_insertions());
    printf("\n");
    assert(test_middle_insertions());

    cvector_free(add_buffer);

    return 0;
}
