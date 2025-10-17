#include "../nvtree.h"

#define NUM_VALUES 1'00'000

int main()
{
    nv_pool_index tree = nv_tree_init();
    struct nv_node node;

    for (size_t i = 1; i <= NUM_VALUES; i++) {
        node.index = i;
        node.length = i;

        nv_pool_index new_root = nv_tree_insert(tree, node);

        tree = nv_tree_paint(new_root, B);
    }

    nv_tree_print(tree);

    // TODO: verify if the tree is actually correct

    nv_tree_free_all(tree);

    return 0;
}
