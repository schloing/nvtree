#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../nvtree.h"

void assign_insert_print(nv_pool_index* tree, const char* string, size_t pos)
{
    struct nv_node data = { .length = strlen(string) };

    *tree = nv_tree_insert(*tree, pos, data);
    *tree = nv_tree_paint(*tree, B);

    nv_tree_print(*tree);
}

int test_try_right_insertions()
{
    printf("attempting right insertions\n");

    nv_pool_index tree = nv_tree_init();

    assign_insert_print(&tree, "monday\n", 10);
    assign_insert_print(&tree, "tuesday\n", 30);
    assign_insert_print(&tree, "wednesday\n", 50);

    nv_tree_free_all(tree);

    return 1;
}

int test_try_left_insertions()
{
    printf("attempting left insertions\n");

    nv_pool_index tree = nv_tree_init();

    assign_insert_print(&tree, "wednesday\n", 50);
    assign_insert_print(&tree, "tuesday\n", 30);
    assign_insert_print(&tree, "monday\n", 10);

    nv_tree_free_all(tree);

    return 1;
}

int main()
{
    assert(test_try_right_insertions());
    assert(test_try_left_insertions());

    return 0;
}
