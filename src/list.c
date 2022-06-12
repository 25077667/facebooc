#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

Node* insert(const void* restrict value, size_t size, const Node* restrict next) {
	Node* new_node = malloc(sizeof(Node));
	new_node->value = malloc(size);
	memcpy(new_node->value, value, size);
	new_node->next = (Node*)next;
	return new_node;
}

Node* insert_move(void* restrict value, const Node* restrict next) {
	Node* new_node = malloc(sizeof(Node));
	new_node->value = value;
	new_node->next = (Node*)next;
	value = NULL;
	return new_node;
}

void clear(Node* node, List_op free_func) {
	while(node != NULL) {
		const Node* tmp = node->next;
		free_func(node->value);
		free(node);
		node = (Node*)tmp;
	}
}

_Bool listForEach(Node* node, List_op func) {
	_Bool result = true;
	Node** indrect = &node;

	while(*indrect != NULL && result) {
		result = func((void*)(*indrect)->value);  // Do it
		indrect = (Node**)&((*indrect)->next);	  // To next
	}

	return result;
}

Node* reverse(Node* head) {
	Node* cur = head;
	Node* prev = NULL;

	while(cur) {
		Node* tmp = cur->next;
		cur->next = prev;
		prev = cur;
		cur = tmp;
	}

	return prev;
}