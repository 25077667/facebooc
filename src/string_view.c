#include "string_view.h"
#include "utility.h"

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif

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