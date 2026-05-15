# nvtree
persistent rb-tree library for not-vim
- persistence with structural sharing
- swappable allocator
- user defined data
- no deletions, sorting, lookups, caching
- comes with optional abstraction to represent text spans

## how to use
modify rb_conf.h and define your data structure
```c
// rb_conf.h
// this file is automatically included in rb.h

// do not change the #define
#ifndef NV_TREE_RB_CONF
#define NV_TREE_RB_CONF

// define data structure
struct nv_tree_data_s {
    size_t value;
};

#endif
```
...then define comparison functions based on your data structure
```c
// your_source.c

bool nv_rb_tree_data_eq(nv_tree_data* a, nv_tree_data* b)
{
    return a->value == b->value;
}

bool nv_rb_tree_data_gt(nv_tree_data* a, nv_tree_data* b) { /* ... */ }
bool nv_rb_tree_data_lt(nv_tree_data* a, nv_tree_data* b)  { /* ... */ }
```
if you want the nvtree (specialised for text spans), compile with nvtree.c and use the default rb_conf.h.

## compilation
TODO: this isn't valid for the latest version
### static library
```sh
make libnvtree.a
```
### test
```sh
make test
```
### massif heap profiler
```sh
make massif
```

## read
- [Red-Black Trees in a Functional Setting](https://doi.org/10.1017/S0956796899003494)
- [Piece Tables, Splay Trees, and "Trables"](https://www.averylaird.com/programming/piece-table/2018/05/10/insertions-piece-table.html)
- [Improving the AbiWord's Piece Table](http://e98cuenc.free.fr/wordprocessor/piecetable.html)
