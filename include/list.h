#ifndef LIST_H
#define LIST_H

#include <stdbool.h>
#include <stddef.h>

// TODO: Generalize this name of "function"
// A function takes pointer and returns a bool,
// which operate element of a list
typedef _Bool (*List_op)(void*);

typedef struct node {
	struct node* next;

	void* value;
} Node;

Node* insert(const void* restrict value, size_t size, const Node* restrict next);
Node* insert_move(void* restrict value, const Node* restrict next);

// head will be NULL
// If your value is non-standard type, please free it by yourself
void clear(Node* head, List_op free_func);
_Bool listForEach(Node* head, List_op func);
Node* reverse(Node* head);

#endif
