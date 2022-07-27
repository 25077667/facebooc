#include "header.h"
#include "helper.h"
#include "list.h"
#include "utility.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// It's a list of SPairs
struct Header {
	SSPair* entry;
	List list;
};

static unsigned int RESP_SIZE = 2048;

// Delete a single node
static inline void Header_del__(Header* h);

#define SSPair(node) ((SSPair*)node->valuess)

static inline void strip_head_spaces(struct string_view* sv) {
	while(isspace(*sv->begin) && sv->begin != sv->end) {
		sv->begin++;
		CONST_INIT(sv->size, sv->size - 1);
	}

	if(sv->begin == sv->end)
		CONST_INIT(sv->size, -1L);
}

// RFC2616 section 4.2
Header* header_parser(char* restrict buf, size_t* restrict offset) {
	Header* h = header_new();
	char* base = buf;

	while(*buf) {  // Read all until the end
		// Read line end with CRLF
		struct string_view line = string_view_ctor(buf, "\r\n");

		if(line.size == 0)	// empty line, that is the end of header
			break;

		const char* seperator = find_first_of(line.begin, ":");

		struct string_view key = { .begin = line.begin,
								   .end = seperator,
								   .size = seperator - line.begin };
		struct string_view value = { .begin = seperator + 1,
									 .end = line.end,
									 .size = line.end - (seperator + 1) };
		CONST_INIT(*key.end, (char)0);	  // Force setting ':' to '\0'
		CONST_INIT(*value.end, (char)0);  // Force setting '\r' to '\0'

		// key is case-insensitive
		to_lower_case((char*)key.begin);
		strip_head_spaces(&value);

		Header* new_node = header_new();
		new_node->entry = (SSPair*)make_pair(&key, &value);
		List_insert_tail(&h->list, &new_node->list);

		buf += line.size + 2;  // Skip '\r' and '\n'
	}

	// Return offset to the caller(aka. request)
	// The request_line already have some value
	*offset += buf + 2 - base;

	return h;
}

Header* header_new() {
	Header* h = malloc(sizeof(Header));
	h->entry = NULL;
	List_ctor(&h->list);

	return h;
}

void header_insert(Header* restrict h, const SSPair* restrict p) {
	if(unlikely(!h || !p))
		return;

	// Deep copy the pair
	SSPair* new_p = malloc(sizeof(SSPair));
	new_p->key = strdup(p->key);
	new_p->value = strdup(p->value);

	Header* new_node = header_new();
	new_node->entry = new_p;
	List_insert_tail(&h->list, &new_node->list);
}

void header_insert_move(Header* restrict h, SSPair* restrict p) {
	if(unlikely(!h || !p))
		return;

	Header* new_node = header_new();
	new_node->entry = p;
	List_insert_tail(&h->list, &new_node->list);
	p = NULL;
}

const char* header_get(const Header* restrict h, const char* restrict key) {
	if(unlikely(!h || !key || !*key))
		return NULL;

	List* cur = NULL;
	list_for_each(cur, &h->list) {
		Header* h_node = container_of(cur, Header, list);
		if(strcmp(h_node->entry->key, key) == 0)
			return h_node->entry->value;
	}
	return NULL;
}

void header_delete(Header* h) {
	if(unlikely(!h))
		return;

	List *node = NULL, *safe = NULL;
	list_for_each_safe(node, safe, &h->list) {
		Header* h_node = container_of(node, Header, list);
		Header_del__(h_node);
	}

	// Delete head, which is an empty node
	Header_del__(h);
}

// The ': \r\n' should be already malloced outside, + 4
static inline void header_entry_to_string__(char* restrict dst, size_t len,
											const SSPair* restrict c) {
	snprintf(dst, len, "%s: %s\r\n", c->key, c->value);
}

char* header_to_string(const Header* h) {
	if(!h || List_is_empty(&h->list))
		return NULL;

	char* sbuf = malloc(RESP_SIZE);
	int index = 0;

	List* cur = NULL;
	list_for_each(cur, &h->list) {
		Header* h_node = container_of(cur, Header, list);
		const size_t key_len = strlen(h_node->entry->key);
		const size_t value_len = strlen(h_node->entry->value);
		const size_t entry_len = key_len + value_len + 5;  // Add ": \r\n"
		if(unlikely(entry_len + index > RESP_SIZE)) {
			sbuf = realloc(sbuf, (RESP_SIZE <<= 1));
		}

		header_entry_to_string__(sbuf + index, entry_len, h_node->entry);
		index += entry_len - 1;
	}

	return sbuf;
}

static inline void Header_del__(Header* h) {
	if(unlikely(!h))
		return;

	SSPair_delete(h->entry);
	free(h);

	h = NULL;
}

#ifdef DEBUG
void Header_print(const Header* h) {
	if(!h || List_is_empty(&h->list))
		return;

	List* cur = NULL;
	list_for_each(cur, &h->list) {
		Header* h_node = container_of(cur, Header, list);
		fprintf(stderr, "%s:%s\n", h_node->entry->key, h_node->entry->value);
	}
}
#endif