#ifndef TREE_ADT_H
#define TREE_ADT_H

#include <stddef.h>

/*
`tree_t` is a data type for tree data structure.

`tree.adt.h` provides the interface of this type.
`tree.adt.c` contains the actual definitions of this type
and the functions for handling a tree.

Meanwhile, since this type is designed to be an abstract
data type, the header file only provides this incomplete
type in order to hide its actual implementation.
*/
typedef struct tree tree_t;

/*
`tree_statcode_t` is an enumeration type for indicating
whether an operation on a tree has been successful
or not.

`tree_failed` represents a failure on an operation.
`tree_ok` represents a success.
*/
typedef enum tree_statcode {
   tree_failed,
   tree_ok
} tree_statcode_t;

/*
< Table of Contents >

The following functions are the operations for
handling a tree.

1. tree_plant        : makes a new tree
2. tree_prune        : destroys a tree
3. tree_addchild     : add a new child tree to a parent tree
4. tree_walk         : traverses a tree
*/

/**
 * @brief `tree_plant` makes a new tree with `data`.
 * @param data a data which this tree is to have
 * @param data_size the size of the data
 * @return the pointer to the tree having been created.
 * @note In failure, this function stops the program.
 */
tree_t *tree_plant(void *data, size_t data_size);

/**
 * @brief `tree_prune` destroys a tree.
 * @param tree a tree to destroy
 */
void tree_prune(tree_t *tree);

/**
 * @brief `tree_addchild` makes a new tree with `data` and
 *    makes the tree the child of `tree`.
 * @param tree a tree which is to be the parent of a child tree
 * @param data a data which the child tree is to have
 * @param data_size the size of the data
 * @return the pointer to the child tree
 * @note In failure, this function stops the program.
 */
tree_t *tree_addchild(tree_t *tree, void *data, size_t data_size);

/**
 * @brief `tree_walk` traverses the whole `tree`. Starting
 *    from the root tree, it depends on `method` the way
 *    this function visits each child tree.
 * @param tree a root tree where a traversal begins
 * @param method a method to walk the tree
 * @param callback a callback function which is to be called
 *    when a tree gets a visit
 * @note Currently only one method is available: `dfs-pre`.
 *    This methods performs a depth-first search with pre-order.
 */
void tree_walk(tree_t *tree, const char *method, void (*callback)(void *data));

#endif