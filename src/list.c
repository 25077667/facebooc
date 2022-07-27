#include "list.h"
#include "utility.h"

#include <stdlib.h>

void List_ctor(List* head) {
	head->next = head;
	head->prev = head;
}

void List_insert_head(List* restrict head, List* restrict new_node) {
	if(unlikely(!head) || unlikely(!new_node))
		return;

	List* next = head->next;

	next->prev = new_node;
	new_node->next = next;
	new_node->prev = head;
	head->next = new_node;
}

void List_insert_tail(List* restrict head, List* restrict new_node) {
	List* prev = head->prev;

	prev->next = new_node;
	new_node->next = head;
	new_node->prev = prev;
	head->prev = new_node;
}

int List_size(const List* head) {
	int counter = 0;

	const List* cur = NULL;
	list_for_each(cur, head) {
		++counter;
	}

	return counter;
}

int List_is_empty(const List* head) {
	if(unlikely(!head))
		return 1;

	return head->next == head;
}