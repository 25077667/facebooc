// This is a circular linked-list implementation

// Partial of codes are copy from: https://github.com/sysprog21/lab0-c/blob/master/list.h

#ifndef LIST_H
#define LIST_H
#include "utility.h"

#include <stdbool.h>
#include <stddef.h>

// List head would not truly store anything
typedef struct List {
	struct List* next;
	struct List* prev;
} List;

// Init an empty list
void List_ctor(List* head);

void List_insert_head(List* restrict head, List* restrict new_node);

void List_insert_tail(List* restrict head, List* restrict new_node);

// Return size, 0 if it is empty
int List_size(const List* head);

int List_is_empty(const List* head);

/**
 * list_for_each - Iterate over list nodes
 * @node: list_head pointer used as iterator
 * @head: pointer to the head of the list
 *
 * The nodes and the head of the list must be kept unmodified while
 * iterating through it. Any modifications to the the list will cause undefined
 * behavior.
 */
#define list_for_each(node, head) for(node = (head)->next; node != (head); node = node->next)

/**
 * list_for_each_safe - Iterate over list nodes and allow deletions
 * @node: list_head pointer used as iterator
 * @safe: list_head pointer used to store info for next entry in list
 * @head: pointer to the head of the list
 *
 * The current node (iterator) is allowed to be removed from the list. Any
 * other modifications to the the list will cause undefined behavior.
 */
#define list_for_each_safe(node, safe, head) \
	for(node = (head)->next, safe = node->next; node != (head); node = safe, safe = node->next)

#endif
