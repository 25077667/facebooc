#include "http/header.h"
#include "http/helper.h"
#include "list.h"
#include "utility.h"
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// It's a list of SPairs
// TODO: Use an array of SPairs and link them together to make the cache more friendly
struct Header {
	Node* head;
};

#define SSPair(node) ((SSPair*)node->value)

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
	Header* h = malloc(sizeof(Header));
	h->head = NULL;
	char* base = buf;

	while(*buf) {  // Read all until the end
		// Read line end with CRLF
		struct string_view line = string_view_ctor(buf, "\r\n");

		if(line.size == 0)	// empty line, that is the end of header
			break;

		const char* seperator = find_first_of(line.begin, ":");
		assert(seperator);

		struct string_view key = { .begin = line.begin,
								   .end = seperator,
								   .size = seperator - line.begin };
		struct string_view value = { .begin = seperator + 1,
									 .end = line.end,
									 .size = line.end - (seperator + 1) };
		CONST_INIT(*key.end, (char)0);	  // Force set ':' to '\0'
		CONST_INIT(*value.end, (char)0);  // Force set '\r' to '\0'

		// key is case-insensitive
		to_lower_case((char*)key.begin);
		strip_head_spaces(&value);

		SSPair* entry = (SSPair*)make_pair(&key, &value);
		h->head = insert_move(entry, h->head);

		buf += line.size + 2;  // Skip '\r' and '\n'
	}

	// Return offset to the caller(aka. request)
	// The request_line already have some value
	*offset += buf + 2 - base;

	return h;
}

Header* header_new() {
	Header* h = malloc(sizeof(Header));
	h->head = NULL;
	return h;
}

void header_insert(Header* restrict h, const SSPair* restrict p) {
	if(unlikely(!h || !p))
		return;

	SSPair* new_p = malloc(sizeof(SSPair));
	new_p->key = strdup(p->key);
	new_p->value = strdup(p->value);
	h->head = insert_move(new_p, h->head);
}

void header_insert_move(Header* restrict h, SSPair* restrict p) {
	if(unlikely(!h || !p))
		return;

	h->head = insert_move(p, h->head);
	p = NULL;
}

void* header_get(const Header* restrict h, const char* restrict key) {
	if(unlikely(!h || !key || !*key))
		return NULL;

	Node* cur = h->head;
	while(cur && strcmp(SSPair(cur)->key, key))
		cur = cur->next;

	if(cur)	 // Found
		return SSPair(cur)->value;
	else
		return NULL;
}

static bool header_entry_dtor__(void* p_) {
	SSPair_delete((SSPair*)p_);
	return true;
}

void header_delete(Header* h) {
	if(unlikely(!h))
		return;

	clear(h->head, header_entry_dtor__);
	free(h);
	h = NULL;
}

// The ': \r\n' should be already malloced outside, + 4
static inline void header_entry_to_string__(char* restrict dst, size_t len,
											const SSPair* restrict c) {
	snprintf(dst, len, "%s: %s\r\n", c->key, c->value);
}

unsigned int RESP_SIZE = 2048;

char* header_to_string(const Header* h) {
	if(!h || !h->head)
		return NULL;

	char* sbuf = malloc(RESP_SIZE);
	int index = 0;

	Node* entry = h->head;
	while(entry) {
		const char* key = SSPair(entry)->key;
		const char* value = SSPair(entry)->value;
		const size_t key_len = strlen(key);
		const size_t value_len = strlen(value);
		const size_t entry_len = key_len + value_len + 5;  // Add ': \r\n'
		if(unlikely(entry_len + index > RESP_SIZE)) {
			sbuf = realloc(sbuf, (RESP_SIZE <<= 1));
			continue;
		}

		header_entry_to_string__(sbuf + index, entry_len, (const SSPair*)entry->value);
		index += entry_len - 1;
		entry = entry->next;
	}
	return sbuf;
}