#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

List* insert(const void* restrict value, size_t size, const List* restrict next) {
	List* new_node = malloc(sizeof(List));
	new_node->value = malloc(size);
	memcpy(new_node->value, value, size);
	new_node->next = (List*)next;
	return new_node;
}

List* insert_move(void* restrict value, const List* restrict next) {
	List* new_node = malloc(sizeof(List));
	new_node->value = value;
	new_node->next = (List*)next;
	value = NULL;
	return new_node;
}

void clear(List* node, List_op free_func) {
	while(node != NULL) {
		const List* tmp = node->next;
		free_func(node->value);
		free(node);
		node = (List*)tmp;
	}
}

_Bool listForEach(List* node, List_op func) {
	_Bool result = true;
	List** indrect = &node;

	while(*indrect != NULL && result) {
		result = func((void*)(*indrect)->value);  // Do it
		indrect = (List**)&((*indrect)->next);	  // To next
	}

	return result;
}

List* reverse(List* head) {
	List* cur = head;
	List* prev = NULL;

	while(cur) {
		List* tmp = cur->next;
		cur->next = prev;
		prev = cur;
		cur = tmp;
	}

	return prev;
}