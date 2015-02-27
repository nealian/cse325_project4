#ifndef __LIST__H__
#define __LIST__H__

/* Element of a doubly linked list */
typedef struct list_elem {
	struct list_elem *prev;
	struct list_elem *next;
	void             *datum;
} list_elem_t;

/* Doubly linked list */
typedef struct list {
	list_elem_t *head;
	list_elem_t *tail;
} list_t;

/* Initialize a list with NULL head and tail. */
void list_init        (list_t *lst);
/* Compute the size of a list (O(n) time). */
int  list_size        (list_t *lst);
/* Initializes a list element with the given datum value
 * and NULL prev and next. */
void list_elem_init   (list_elem_t *elt, void *datum);
/* Insert an element at the head of a list.
 * When this function returns, lst->head == elt */
void list_insert_head (list_t *lst, list_elem_t *elt);
/* Insert an element at the tail of a list.
 * When this function returns, lst->tail == elt */
void list_insert_tail (list_t *lst, list_elem_t *elt);
/* Returns the element at the head of the list (or NULL if empty). */
list_elem_t *list_get_head(list_t *lst);
/* Returns the element at the tail of the list (or NULL if empty). */
list_elem_t *list_get_tail(list_t *lst);
/* Removes an element from a list (assuming it's in the list). */
void list_remove_elem (list_t *lst, list_elem_t *elt);
/* Applies function fn to each element of the list in order, starting from
 * the head.  Useful, for example, to print out the contents of a list. */
void list_foreach     (list_t *lst, void (*fn)(list_elem_t *));

#endif /* __LIST__H__ */
