#ifndef NV_TREE_RB_CONF
#define NV_TREE_RB_CONF

#include <stddef.h>

struct nv_tree_data_s {
    size_t size_left;
    size_t size;
    size_t line_count;
    size_t buff_index;
};

#endif