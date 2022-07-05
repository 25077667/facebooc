#include "helper.h"

#include "utility.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif

static inline int max(int a, int b) {
	return a > b ? a : b;
}

static inline void badCharHeuristic(const char* str, size_t size, char badchar[256]) {
	memset(badchar, -1, 256);

	for(size_t i = 0; i < size; i++)
		badchar[(int)str[i]] = i;
}

// pat is non-empty
// https://www.geeksforgeeks.org/boyer-moore-algorithm-for-pattern-searching/
// Return NULL if not found
static char* boyer_moore(const char* txt, const char* pat) {
	size_t m = strlen(pat);
	size_t n = strlen(txt);

	/* Quick edge cases. */
	if(m == 0 || m > n)
		return NULL;
	if(m == 1)
		return strchr(txt, *pat);

	char badchar[256];
	badCharHeuristic(pat, m, badchar);

	size_t s = 0;
	while(s <= n - m) {
		long j = m - 1;
		while(j >= 0 && pat[j] == txt[s + j])
			j--;
		if(j < 0)
			return (char*)txt + s - 1;
		else
			s += max(1, j - badchar[(int)txt[s + j]]);
	}

	return NULL;
}

// https://stackoverflow.com/a/14530993
static void urldecode2(char* dst, const char* src) {
	char a, b;
	while(*src) {
		if((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
			if(a >= 'a')
				a -= 'a' - 'A';
			if(a >= 'A')
				a -= ('A' - 10);
			else
				a -= '0';
			if(b >= 'a')
				b -= 'a' - 'A';
			if(b >= 'A')
				b -= ('A' - 10);
			else
				b -= '0';
			*dst++ = 16 * a + b;
			src += 3;
		}
		else if(*src == '+') {
			*dst++ = ' ';
			src++;
		}
		else {
			*dst++ = *src++;
		}
	}
	*dst++ = '\0';
}

char* string_view_dup(const struct string_view* const sv) {
	char* copy = NULL;
	if(sv->size != -1) {
		copy = malloc(sv->size + 1);
		strncpy(copy, sv->begin, sv->size);
		copy[sv->size] = 0;
	}
	else {
		copy = malloc(1);
		*copy = 0;
	}
	return copy;
}

void to_lower_case(char* str) {
	while(str && *str) {
		*str = tolower(*str);
		str++;
	}
}

struct string_view string_view_ctor(const char* buf, const char* delim) {
	long size = -1;
	const char* end = find_first_of(buf, delim) + 1;
	if(end != (void*)1)
		size = end - buf;

	struct string_view sv = { .begin = buf, .end = end, .size = size };
	return sv;
}

#ifdef DEBUG
void string_view_show(const struct string_view* const sv) {
	if(sv->size == -1)
		return;

	char* copy = malloc(sv->size + 1);
	memcpy(copy, sv->begin, sv->size);
	copy[sv->size] = 0;
	fprintf(stderr, "%s\n", copy);
	free(copy);
}
#endif

// Return NULL if not found
char* find_first_of(const char* str, const char* delim) {
	if(str && *str)
		return boyer_moore(str, delim);
	else
		return NULL;
}

// Support unicode
char* url_decoder(const char* str) {
	// Each '%' symbol will brought another 2 characters behind it
	int percent_counter = 0;
	char* shadow_str = (char*)str;
	while((*shadow_str++))
		if(*shadow_str == '%')
			++percent_counter;

	char* new_url = malloc(strlen(str) - percent_counter * 2 + 1);

	urldecode2(new_url, str);
	return new_url;
}

SPair* make_pair(const struct string_view* const key, const struct string_view* const value) {
	SPair* entry = malloc(sizeof(SPair));
	entry->key = string_view_dup(key);
	entry->value = string_view_dup(value);

	return entry;
}

// Case sensitive
SPair* query_entry(const char* str) {
	const char* seperator = find_first_of(str, "=");
	struct string_view key = { .begin = str, .end = seperator, .size = seperator - str };
	struct string_view value = { .begin = seperator + 1,
								 .end = str + strlen(str) + 1,
								 .size = strlen(str) - key.size - 1 };
	return make_pair(&key, &value);
}