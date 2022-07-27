#include "body.h"
#include "helper.h"

#include "list.h"
#include "utility.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif

// It's a list of SPairs
// TODO: Use an array of SPairs and link them together to make the cache more friendly
struct Body {
	SPair* entry;

	List list;
};

// If you want to implement Content-Type please refer RFC 2616 7.1

static inline _Bool is_supported_type(const char* str);
static inline _Bool check_header(const Header* header);

// Delete a single node
static inline void Body_del__(Body* b);
static inline Body* Body_new(const SPair* p);

// Similar to query_parser
Body* body_parser(const Header* restrict header, char* restrict buffer) {
	if(!check_header(header) || !buffer)
		return NULL;

	Body* b = Body_new(NULL);

	char* decoded = url_decoder(buffer);
	char* decoded_shadow = decoded;
	char* decoded_end__ = decoded + strlen(decoded);
	decoded_end__[0] = '&';
	decoded_end__[1] = 0;

	while(*decoded) {
		SPair* new_entry = pair_lexer(decoded, '=', '&');
		if(new_entry == NULL)
			break;

		Body* new_node = Body_new(new_entry);
		List_insert_head(&b->list, &new_node->list);

		decoded = strchr(decoded, '&') + 1;
	}
	free(decoded_shadow);

	return b;
}

void* body_get(const Body* restrict b, const char* restrict key) {
	if(unlikely(!b || !key || !*key))
		return NULL;

	List* cur = NULL;
	list_for_each(cur, &b->list) {
		Body* b_node = container_of(cur, Body, list);
		if(strcmp(b_node->entry->key, key) == 0)
			return b_node->entry->value;
	}
	return NULL;
}

void body_delete(Body* b) {
	if(!b)
		return;

	List *node = NULL, *safe = NULL;
	list_for_each_safe(node, safe, &b->list) {
		Body* b_node = container_of(node, Body, list);
		Body_del__(b_node);
	}

	// Delete head, which is an empty node
	Body_del__(b);
}

static inline _Bool is_supported_type(const char* str) {
	if(!str)
		return false;
	return !strncmp(str, "application/x-www-form-urlencoded", 34);
}

static inline _Bool check_header(const Header* header) {
	const char* content_type = header_get(header, "content-type");
	if(content_type)
		return is_supported_type(content_type);
	return false;
}

static inline void Body_del__(Body* b) {
	if(unlikely(!b))
		return;

	SPair_delete(b->entry);
	free(b);

	b = NULL;
}

static inline Body* Body_new(const SPair* p) {
	Body* b = malloc(sizeof(Body));
	b->entry = (SPair*)p;
	List_ctor(&b->list);

	return b;
}

#ifdef DEBUG
void Body_print(const Body* b) {
	if(unlikely(!b))
		return;

	List* cur = NULL;
	list_for_each(cur, &b->list) {
		Body* b__ = container_of(cur, Body, list);
		fprintf(stderr, "%s:%s\n", b__->entry->key, (char*)b__->entry->value);
	}
}
#endif