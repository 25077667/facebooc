#include "query.h"
#include "helper.h"
#include "list.h"
#include "pair.h"
#include "string_view.h"
#include "utility.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif

struct Query {
	SSPair* entry;

	List list;
};

static inline Query* Query_new(const SSPair* p);

// Delete a single node
static inline void Query_del__(Query* q);

Query* query_parser(const char* buffer) {
	// Create head as an empty node
	Query* q = Query_new(NULL);

	char* decoded = url_decoder(buffer);
	char* decoded_shadow = decoded;
	char* decoded_end__ = decoded + strlen(decoded);
	decoded_end__[0] = '&';
	decoded_end__[1] = 0;

	while(*decoded) {
		SSPair* new_entry = (SSPair*)pair_lexer(decoded, '=', '&');
		if(new_entry == NULL)
			break;

		Query* new_node = Query_new(new_entry);
		List_insert_head(&q->list, &new_node->list);

		decoded = strchr(decoded, '&') + 1;
	}
	free(decoded_shadow);

	return q;
}

const char* query_get(const Query* restrict q, const char* restrict key) {
	if(unlikely(!q || !key || !*key))
		return NULL;

	List* cur = NULL;
	list_for_each(cur, &q->list) {
		Query* q_node = container_of(cur, Query, list);
		if(strcmp(q_node->entry->key, key) == 0)
			return q_node->entry->value;
	}
	return NULL;
}

void query_delete(Query* q) {
	if(likely(!q))	// Query string is not often happened
		return;

	List *node = NULL, *safe = NULL;
	list_for_each_safe(node, safe, &q->list) {
		Query* q_node = container_of(node, Query, list);
		Query_del__(q_node);
	}

	// Delete head, which is an empty node
	Query_del__(q);
}

static inline Query* Query_new(const SSPair* p) {
	Query* q = malloc(sizeof(Query));
	q->entry = (SSPair*)p;
	List_ctor(&q->list);

	return q;
}

static inline void Query_del__(Query* q) {
	if(unlikely(!q))
		return;

	SSPair_delete(q->entry);
	free(q);
	q = NULL;
}