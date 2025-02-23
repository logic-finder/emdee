#include <stdlib.h>
#include <string.h>
#include "list.adt.h"
#include "misc.h"

/*
`node_t` is a type for representing an element which
constitutes a list.

`prev` is the pointer to the previous element.
`next` is the pointer to the next element.
`data` is the pointer to an object.
`data_size` is the size of the object pointed by `data`.
*/
typedef struct node {
   struct node *prev;
   struct node *next;
   void *data;
   size_t data_size;
} node_t;

/*
`struct list` is the actual definition of the `list_t` type declared in `list.h`.

`head` is the pointer to the first element.
`tail` is the pointer to the last element.
`size` is the length of this list.
*/
struct list {
   node_t *head;
   node_t *tail;
   int size;
};

extern list_t *list_create(void) {
   list_t *list;

   list = salloc(sizeof *list);
   
   list->head = NULL;
   list->tail = NULL;
   list->size = 0;

   return list;
}

extern void list_destroy(list_t *list) {
   list_cleanup(list);
   free(list);
}

/**
 * @brief `make_node` does the following tasks:
 *    1. It makes a `node_t` object.
 *    2. It copies the object pointed by `item`.
 *    3. It stores the copied data into the node.
 *    4. It returns the node.
 * @param item a pointer to an object to copy from; it must not be NULL.
 * @param item_size the size of the object to be copied; it must not be 0.
 * @return a pointer to a `node_t` object whose data is copied from `item`.
 */
static node_t *make_node(void *item, size_t item_size);

extern void *list_push(list_t *list, void *item, size_t item_size) {
   /*
   BEFORE
   node --> NULL
   (tail)
   
   AFTER
   node <--> new_node --> NULL
             (tail)
   */

   node_t *new_node = make_node(item, item_size);

   if (list->size == 0) {
      list->tail = new_node;
      list->head = new_node;
      new_node->prev = NULL;
      new_node->next = NULL;
   }
   else {
      list->tail->next = new_node;
      new_node->prev = list->tail;
      new_node->next = NULL;
      list->tail = new_node;
   }
   list->size++;

   return new_node->data;
}

static node_t *make_node(void *item, size_t item_size) {
   node_t *new_node = salloc(sizeof *new_node);
   void *item_copied = salloc(item_size);
   
   memcpy(item_copied, item, item_size);

   new_node->data = item_copied;
   new_node->data_size = item_size;

   return new_node;
}

extern void *list_unshift(list_t *list, void *item, size_t item_size) {
   /*
   BEFORE
   NULL <-- node
            (head)
   
   AFTER
   NULL <-- new_node <--> node
            (head)
   */

   node_t *new_node = make_node(item, item_size);

   if (list->size == 0) {
      list->tail = new_node;
      list->head = new_node;
      new_node->prev = NULL;
      new_node->next = NULL;
   }
   else {
      list->head->prev = new_node;
      new_node->next = list->head;
      new_node->prev = NULL;
      list->head = new_node;
   }
   list->size++;

   return new_node->data;
}

/**
 * @brief `get_node` finds the `index`th node.
 * @param list a list to search for; it must not be NULL.
 * @param index range: 0 <= index < list.length
 * @return the pointer to the node; if the list is empty, it returns NULL.
 */
static node_t *get_node(list_t *list, int index);

extern void *list_insert(list_t *list, int index, void *item, size_t item_size) {
   /*
   BEFORE
   prev_node <--> next_node
   (index-1)      (index)
   
   AFTER
   prev_node <--> new_node <--> next_node
   (index-1)      (index)       (index+1)
   */

   if (index == 0)
      return list_unshift(list, item, item_size);
   if (index == list->size)
      return list_push(list, item, item_size);
   
   node_t *prev_node, *new_node, *next_node;

   prev_node = get_node(list, index - 1);
   new_node = make_node(item, item_size);
   next_node = prev_node->next;
   
   new_node->next = next_node;
   next_node->prev = new_node;
   new_node->prev = prev_node;
   prev_node->next = new_node;

   list->size++;

   return new_node->data;
}

static node_t *get_node(list_t *list, int index) {
   node_t *node = list->head;

   if (!node)
      return NULL;
   
   int count = 0;

   while (count++ < index)
      node = node->next;

   return node;
}

extern list_statcode_t list_pop(list_t *list, void *dest) {
   /*
   BEFORE
   node <--> node --> NULL
             (tail)
   
   AFTER
   node --> NULL
   (tail)
   */
  
   node_t *current_tail = list->tail;
   
   if (!current_tail)
      return list_failed;
   if (dest)
      memcpy(dest, current_tail->data, current_tail->data_size);

   if (list->size == 1) {
      free(current_tail->data);
      free(current_tail);
      list->head = NULL;
      list->tail = NULL;
   }
   else {
      node_t *new_tail = current_tail->prev;

      new_tail->next = NULL;
      free(current_tail->data);
      free(current_tail);
      list->tail = new_tail;
   }
   list->size--;

   return list_ok;
}

extern list_statcode_t list_shift(list_t *list, void *dest) {
   node_t *current_head = list->head;

   if (!current_head)
      return list_failed;
   if (dest)
      memcpy(dest, current_head->data, current_head->data_size);

   if (list->size == 1) {
      free(current_head->data);
      free(current_head);
      list->head = NULL;
      list->tail = NULL;
   }
   else {
      node_t *new_head = current_head->next;

      new_head->prev = NULL;
      free(current_head->data);
      free(current_head);
      list->head = new_head;
   }
   list->size--;

   return list_ok;
}

extern list_statcode_t list_erase(list_t *list, int index, void *dest) {
   if (index == 0)
      return list_shift(list, dest);
   if (index == (list->size - 1))
      return list_pop(list, dest);

   node_t *prev_node, *current_node, *next_node;

   current_node = get_node(list, index);
   prev_node = current_node->prev;
   next_node = current_node->next;

   if (dest)
      memcpy(dest, current_node->data, current_node->data_size);

   prev_node->next = next_node;
   next_node->prev = prev_node;
   free(current_node->data);
   free(current_node);

   list->size--;

   return list_ok;
}

extern void *list_tail(list_t *list) {
   return list->tail->data;
}

extern void *list_head(list_t *list) {
   return list->head->data;
}

extern void *list_peek(list_t *list, int index) {
   return get_node(list, index)->data;
}

extern int list_size(list_t *list) {
   return list->size;
}

extern void list_cleanup(list_t *list) {
   while (list_pop(list, NULL) == list_ok);
}

extern bool list_empty(list_t *list) {
   return list->size == 0 ? true : false;
}

extern bool list_full(list_t *list) {
   /*
   Since this implementation utilizes a linked list,
   this list has no limit on the number of elements it
   can accommodate unless `malloc()` gets failed.
   */
   return false;
}

extern int list_indexof(list_t *list, void *data) {
   node_t *node = list->head;
   int i;

   for (i = 0; i < list->size; i++)
      if (node->data == data)
         break;
   
   return i;
}

extern void list_foreach(list_t *list, void (*callback)(void *data, int index)) {
   for (int i = 0; i < list->size; i++) {
      void *data = list_peek(list, i);
      callback(data, i);
   }
}