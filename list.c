#include <stdlib.h>

#include "list.h"

void list_init(list_t *lst)
{
	if (lst != NULL)
		lst->head = lst->tail = NULL;
}

int list_size(list_t *lst)
{
	list_elem_t *elt;
	int count = 0;
	for (elt = lst->head; elt != NULL; elt = elt->next)
		count++;
	return count;
}

void list_elem_init(list_elem_t *elt, void *datum)
{
	if (elt != NULL) {
		elt->prev = elt->next = NULL;
		elt->datum = datum;
	}
}

void list_insert_head(list_t *lst, list_elem_t *elt)
{
	if (lst->head == NULL) {
		elt->prev = elt->next = NULL;
		lst->head = lst->tail = elt;
	} else {
		elt->prev = NULL;
		elt->next = lst->head;
		if (lst->head)
			lst->head->prev = elt;
		lst->head = elt;
	}
}

void list_insert_tail(list_t *lst, list_elem_t *elt)
{
	if (lst->tail == NULL) {
		elt->prev = elt->next = NULL;
		lst->head = lst->tail = elt;
	} else {
		elt->prev = lst->tail;
		if (lst->tail)
			lst->tail->next = elt;
		elt->next = NULL;
		lst->tail = elt;
	}
}

list_elem_t *list_get_head(list_t *lst)
{
	return lst->head;
}

list_elem_t *list_get_tail(list_t *lst)
{
	return lst->tail;
}

void list_remove_elem(list_t *lst, list_elem_t *elt)
{
	if (elt->prev) {
		elt->prev->next = elt->next;
	} else if (lst->head == elt) {
		lst->head = elt->next;
	}
	if (elt->next) {
		elt->next->prev = elt->prev;
	} else if (lst->tail == elt) {
		lst->tail = elt->prev;
	}
	elt->next = elt->prev = NULL;
}

void list_foreach(list_t *lst, void (*fn)(list_elem_t *))
{
	list_elem_t *elt;
	for (elt = lst->head; elt != NULL; elt = elt->next)
		fn(elt);
}
