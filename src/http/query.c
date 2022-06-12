#include "http/query.h"
#include "http/helper.h"
#include "list.h"
#include "pair.h"
#include "utility.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif

// It's a list of SPairs
// TODO: Use an array of SPairs and link them together to make the cache more friendly
struct Query {
	Node* head;
};

#define SPAIR(node) ((SPair*)node->value)

Query* query_parser(char* buffer) {
	Query* q = malloc(sizeof(Query));
	q->head = NULL;
	while(*buffer) {
		struct string_view sep = string_view_ctor(buffer, "&");
		if(sep.size != -1) {
			CONST_INIT(*(sep.end - 1), (char)0);
		}

		// Check if it have '='
		if(strchr(buffer, '=')) {  // transfter url encoding
			char* decoded = url_decoder(buffer);
			SPair* entry = query_entry(decoded);
			q->head = insert_move(entry, q->head);
			free(decoded);
		}

		buffer += strlen(buffer) + 1;
	}

	return q;
}

void* query_get(const Query* restrict q, const char* restrict key) {
	if(unlikely(!q || !key || !*key))
		return NULL;

	Node* cur = q->head;
	while(cur && strcmp(SPAIR(cur)->key, key))
		cur = cur->next;

	if(cur)	 // Found
		return SPAIR(cur)->value;
	else
		return NULL;
}

static bool query_entry_dtor__(void* p_) {
	SPair_delete((SPair*)p_);
	return true;
}

void query_delete(Query* q) {
	if(likely(!q))	// Query string is not often happened
		return;

	clear(q->head, query_entry_dtor__);
	free(q);
}