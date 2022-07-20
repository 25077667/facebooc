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
} List;

List* insert(const void* restrict value, size_t size, const List* restrict next);
List* insert_move(void* restrict value, const List* restrict next);

// head will be NULL
// If your value is non-standard type, please free it by yourself
void clear(List* head, List_op free_func);
_Bool listForEach(List* head, List_op func);
List* reverse(List* head);

#endif
