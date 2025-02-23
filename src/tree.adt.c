#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "tree.adt.h"
#include "list.adt.h"
#include "misc.h"

struct tree {
   tree_t *parent;
   list_t *children;
   void *data;
   size_t data_size;
};

extern tree_t *tree_plant(void *data, size_t data_size) {
   tree_t *sapling = salloc(sizeof *sapling);
   void *tree_data = salloc(data_size);
   list_t *tree_children = list_create();

   sapling->parent = NULL;
   sapling->children = tree_children;
   sapling->data = tree_data;
   sapling->data_size = data_size;

   memcpy(sapling->data, data, data_size);

   return sapling;
}

/**
 * @brief `cleanup` wraps `tree_prune`, which allows `list_foreach`
 *    to call `tree_prune` indirectly.
 * @param tree a tree to clean
 * @param _ This parameter is not used.
 */
static void cleanup(void *tree, int _);

extern void tree_prune(tree_t *tree) {
   /* in case of a leaf, since it has no child,
      `cleanup` is not executed. */
   list_foreach(tree->children, cleanup);
   list_destroy(tree->children);
   free(tree->data);
   free(tree);
}

static void cleanup(void *tree, int _) {
   tree_prune((tree_t *) tree);
}

extern tree_t *tree_addchild(tree_t *tree, void *data, size_t data_size) {
   tree_t *leaf;
   
   leaf = tree_plant(data, data_size);
   leaf->parent = tree;
   list_push(tree->children, leaf, sizeof *leaf);  /* copies leaf */
   tree_prune(leaf); /* frees the original */

   return leaf;
}

/**
 * @brief `do_dfs_pre_walk` performs a depth-first search with pre-order.
 * @param tree a root tree to begin a traversal
 * @param callback a callback function to be called on a tree which gets a visit
 */
static void do_dfs_pre_walk(tree_t *tree, void (*callback)(void *data));

extern void tree_walk(tree_t *tree, const char *method, void (*callback)(void *data)) {
   if (strcmp(method, "dfs-pre") == 0)
      do_dfs_pre_walk(tree, callback);
   else
      ERR2("unknown `tree_walk` method: %s.", method);
}

static void do_dfs_pre_walk(tree_t *tree, void (*callback)(void *data)) {
   callback(tree->data);

   const int len = list_size(tree->children);
   if (len == 0)
      return;  /* end of recursion */
   
   for (int i = 0; i < len; i++) {
      tree_t *child = list_peek(tree->children, i);
      do_dfs_pre_walk(child, callback);
   }
}