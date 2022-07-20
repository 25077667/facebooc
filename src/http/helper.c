#include "helper.h"
#include "string_view.h"
#include "utility.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
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
	char* shadow_str = (char*)str;
	while((*shadow_str++))
		if(*shadow_str == '%')
			++percent_counter;

	char* new_url = malloc(strlen(str) - percent_counter * 2 + 1);

	urldecode2(new_url, str);
	return new_url;
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