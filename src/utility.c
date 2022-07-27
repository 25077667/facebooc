#include "utility.h"

#include <ctype.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void badCharHeuristic(const char* str, size_t size, char badchar[256]);
static char* boyer_moore(const char* txt, const char* pat);

// Reference: http://www.cse.yorku.ca/~oz/hash.html
int string_hash(const char* str) {
	int hash = 5381;
	int c;

	while((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

// sdbm
int obj_hash(const void* data, size_t size) {
	unsigned long hash = 0;

	for(size_t i = 0; i < size; i++) {
		hash = ((int*)data)[i] + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

#ifdef DEBUG
void mem_canary_alert(const char* msg) {
	puts(msg);
#ifdef __SANITIZE_ADDRESS__
	char* canary = NULL;
	*canary = 0;
#else
	void* buffer[64];
	char** symbols;

	int num = backtrace(buffer, 64);
	printf("backtrace() returned %d addresses\n", num);

	symbols = backtrace_symbols(buffer, num);
	if(symbols == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	for(int j = 0; j < num; j++)
		printf("%s\n", symbols[j]);

	free(symbols);
#endif
}
#endif

void* memdup(const void* mem, size_t size) {
	void* out = malloc(size);
	if(out != NULL)
		memcpy(out, mem, size);
	return out;
}

void fetch_dir(char* restrict dst, const char* restrict src) {
	const char* sep = strchr(src, '/');
	if(sep)
		memcpy(dst, src, sep - src);
}

void to_lower_case(char* str) {
	while(str && *str) {
		*str = tolower(*str);
		str++;
	}
}

// Return NULL if not found
char* find_first_of(const char* str, const char* delim) {
	if(str && *str)
		return boyer_moore(str, delim);
	else
		return NULL;
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

SPair* make_pair(const struct string_view* const key, const struct string_view* const value) {
	SPair* entry = malloc(sizeof(SPair));
	entry->key = string_view_dup(key);
	entry->value = string_view_dup(value);

	return entry;
}