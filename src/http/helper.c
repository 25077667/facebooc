#include "helper.h"
#include "string_view.h"
#include "utility.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

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

// Support unicode
char* url_decoder(const char* str) {
	// Each '%' symbol will brought another 2 characters behind it
	int percent_counter = 0;
	const char* shadow_str = str;
	while((*shadow_str++))
		if(*shadow_str == '%')
			++percent_counter;

	// Remain more space for some utilities, which needs some other token to make parser easier
	const int REMAIN_SPACE = 16;
	char* new_url = malloc(strlen(str) - percent_counter * 2 + 1 + REMAIN_SPACE);

	urldecode2(new_url, str);
	return new_url;
}

SPair* query_entry(const char* str) {
	const char* seperator = find_first_of(str, "=");
	struct string_view key = { .begin = str, .end = seperator, .size = seperator - str };
	struct string_view value = { .begin = seperator + 1,
								 .end = str + strlen(str) + 1,
								 .size = strlen(str) - key.size - 1 };
	return make_pair(&key, &value);
}

SPair* pair_lexer(const char* str, char delim, char term) {
	SPair* p = NULL;
	if(str == NULL || *str == 0)
		goto ret;

	const char* delim_ptr = strchr(str, delim);
	if(delim_ptr == NULL)
		goto ret;

	const char* term_ptr = strchr(delim_ptr + 1, term);
	if(term_ptr == NULL)
		goto ret;

	string_view key = { .begin = str, .end = delim_ptr, .size = delim_ptr - str };
	string_view value = { .begin = delim_ptr + 1, .end = term_ptr, .size = term_ptr - delim_ptr };

	p = make_pair(&key, &value);

ret:
	return p;
}