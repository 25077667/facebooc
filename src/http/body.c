#include "body.h"
#include "helper.h"
#include "query.h"

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
	List* head;
};

#define SPAIR(node) ((SPair*)node->value)

// If you want to implement Content-Type please refer RFC 2616 7.1

static inline _Bool is_sepported_type(const char* str) {
	if(!str)
		return false;
	return !strncmp(str, "application/x-www-form-urlencoded", 34);
}

static inline _Bool check_header(const Header* header) {
	const char* content_type = header_get(header, "content-type");
	if(content_type)
		return is_sepported_type(content_type);
	return false;
}

Body* body_parser(const Header* restrict header, char* restrict buffer) {
	if(!check_header(header) || !buffer)
		return NULL;

	return (Body*)query_parser(buffer);
}

Body* body_new() {
	Body* b = malloc(sizeof(Body));
	b->head = NULL;
	return b;
}

Body* body_insert(Body* restrict b, const SPair* restrict p) {
	if(unlikely(!b || !p))
		return NULL;
	b->head = insert(p, sizeof(SPair), b->head);
	return b;
}

Body* body_insert_move(Body* restrict b, SPair* restrict p) {
	if(unlikely(!b || !p))
		return NULL;
	b->head = insert_move(p, b->head);
	return b;
}

void* body_get(const Body* restrict b, const char* restrict key) {
	return query_get((const Query*)b, key);
}

void body_delete(Body* b) {
	return query_delete((Query*)b);
}