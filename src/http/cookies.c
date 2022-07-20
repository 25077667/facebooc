#include "cookies.h"
#include "helper.h"
#include "list.h"
#include "pair.h"
#include "string_view.h"
#include "utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char* c_av_fmt_str[] = {
	"",	 // Should be ignoreed (key)
	"",	 // Should be ignoreed (value)
	"; Expires=%s",
	"; Max-Age=%s",
	"; Domain=%s",
	"; Path=%s",
	"; %s",
	"; %s",
};

// rfc6265, Section 4.1.1
// set-cookie-string  = cookie-pair *( ";" SP cookie-av )
// 		   cookie-av  = expires-av / max-age-av / domain-av /
//  					path-av / secure-av / httponly-av /
//  					extension-av
typedef struct __attribute__((__packed__)) Cookie {
	const char* key;
	const char* value;
	const char* expires;
	const char* max_age;
	const char* domain;
	const char* path;
	const char* secure;
	const char* httponly;

} Cookie;

typedef Cookie Cookie_to_string_template__;

struct Cookies {
	List* dict;
};

Cookies* Cookies_new(void) {
	Cookies* c = malloc(sizeof(Cookies));
	c->dict = NULL;
	return c;
}

static inline void single_cookie_init__(Cookie* c) {
	c->key = NULL;
	c->value = NULL;
	c->expires = NULL;
	c->max_age = NULL;
	c->domain = NULL;
	c->path = NULL;
	c->secure = NULL;
	c->httponly = NULL;
}

static inline Cookie* single_cookie_new__(void) {
	Cookie* c = malloc(sizeof(Cookie));
	single_cookie_init__(c);
	return c;
}

static inline void single_cookie_delete__(Cookie* c) {
	free((char*)c->key);
	free((char*)c->value);
	free((char*)c->expires);
	free((char*)c->max_age);
	free((char*)c->domain);
	free((char*)c->path);
	free((char*)c->secure);
	free((char*)c->httponly);
	free(c);
	c = NULL;
}

static _Bool cookie_entry_dtor__(void* e) {
	single_cookie_delete__((Cookie*)e);
	return true;
}

static Cookie* single_cookie_gen__(const char* str) {
	const char* seperator = find_first_of(str, "=");
	struct string_view key = { .begin = str, .end = seperator, .size = seperator - str };
	struct string_view value = { .begin = seperator + 1,
								 .end = str + strlen(str) + 1,
								 .size = strlen(str) - key.size - 1 };
	Cookie* c = single_cookie_new__();
	single_cookie_init__(c);
	c->key = string_view_dup(&key);
	c->value = string_view_dup(&value);

	return c;
}

static inline size_t cookie_template_len__(const Cookie_to_string_template__* t) {
	return strlen(t->key) + strlen(t->value) + strlen(t->expires) + strlen(t->max_age)
		   + strlen(t->domain) + strlen(t->path) + strlen(t->secure) + strlen(t->httponly);
}

Cookies* Cookies_insert(Cookies* c, const char* key, const char* value) {
	if(unlikely(!c || !key || !*key || !value))	 // value could be "" empty string
		return c;

	Cookie* new_c = single_cookie_new__();
	new_c->key = strdup(key);
	new_c->value = strdup(value);

	c->dict = insert_move(new_c, c->dict);
	return c;
}

Cookies* Cookies_init(const char* key, const char* value) {
	return Cookies_insert(Cookies_new(), key, value);
}

#define List_to_Cookie(entry) ((Cookie*)((entry)->value))

Cookie* Cookies_get(const Cookies* c, const char* key) {
	if(unlikely(!c || !key))
		return NULL;

	List* entry = c->dict;
	while(entry) {
		const char* entry_key = List_to_Cookie(entry)->key;
		if(strcmp(key, entry_key)) {  // Key is not matched
			entry = entry->next;
			continue;
		}

		// Is found the key
		return List_to_Cookie(entry);
	}
	return NULL;
}

void Cookies_delete(Cookies* c) {
	if(unlikely(!c))
		return;

	clear(c->dict, cookie_entry_dtor__);
	free(c);
	c = NULL;
}

Cookies* cookies_parser(const Header* header) {
	if(!header)
		return NULL;
	const char* str = header_get(header, "cookie");
	if(!str)  // No cookie
		return NULL;

	// The str is begin with non-whitespace
	// cookie-header    = "Cookie:" OWS cookie-string OWS
	// cookie-string    = cookie-pair *(";" SP cookie-pair)
	Cookies* cookies = Cookies_new();
	while(*str) {
		struct string_view sep = string_view_ctor(str, "; ");
		if(sep.size != -1) {
			CONST_INIT(*(sep.end - 1), (char)0);
		}

		// Check if it have '='
		if(strchr(str, '=')) {	// transfer url encoding
			char* decoded = url_decoder(str);

			Cookie* c = single_cookie_gen__(decoded);
			cookies->dict = insert_move(c, cookies->dict);
			free(decoded);
		}
		str += strlen(str);
	}

	return cookies;
}

void Cookies_set_attr(Cookies* c, const char* key, Cookie_attr attr, const char* attr_value) {
	if(unlikely(!c || !key || !*key || !attr_value || (uint32_t)attr > HTTPONLY))
		return;

	Cookie* cookie = Cookies_get(c, key);
	char** entry = ((char**)cookie) + attr;	 // Use offset to setup the char*
	free(*entry);

	// Becareful, @len will calculate the format string's place holder
	size_t len = strlen(c_av_fmt_str[attr]) + strlen(attr_value);
	char* new_attr_entry = malloc(len);
	snprintf(new_attr_entry, len, c_av_fmt_str[attr], attr_value);

	*entry = new_attr_entry;
}

char* Cookie_to_string(const Cookies* c) {
	if(unlikely(!c || !c->dict))
		return NULL;

	// Suppose there is only one cookie,
	const Cookie* cur_c = List_to_Cookie(c->dict);	// Aliasing
	const Cookie_to_string_template__ t = {
		.key = cur_c->key,
		.value = cur_c->value,
		.expires = cur_c->expires ? cur_c->expires : "",
		.max_age = cur_c->max_age ? cur_c->max_age : "",
		.domain = cur_c->domain ? cur_c->domain : "",
		.path = cur_c->path ? cur_c->path : "; Path=/",	 // Defalut path is root
		.secure = cur_c->secure ? "; Secure" : "",
		.httponly = cur_c->httponly ? "; HttpOnly" : "",
	};

	size_t cur_len = cookie_template_len__(&t);

	char* str = malloc(cur_len + 2);  // Add '=' and '\0'

	snprintf(str, cur_len + 2, "%s=%s%s%s%s%s%s%s", t.key, t.value, t.expires, t.max_age, t.domain,
			 t.path, t.secure, t.httponly);

	return str;
}

// RFC 1123
//  sane-cookie-date  = <rfc1123-date, defined in [RFC2616], Section 3.3.1>
void Cookie_gen_expire(char buf[64], int duration) {
	const time_t t = time(NULL) + duration;
	strftime(buf, 64, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&t));
}

void Cookies_set_expire(Cookies* c, const char* key, int dutation) {
	char sbuf[64];
	Cookie_gen_expire(sbuf, dutation);
	Cookies_set_attr(c, key, EXPIRE, sbuf);
}

void Cookies_print(const Cookies* c) {
	if(!c || !c->dict)
		return;

	List* entry = c->dict;
	while(entry) {
		Cookie* cur_c = List_to_Cookie(entry);
		const Cookie_to_string_template__ t = {
			.key = cur_c->key,
			.value = cur_c->value,
			.expires = cur_c->expires ? cur_c->expires : "",
			.max_age = cur_c->max_age ? cur_c->max_age : "",
			.domain = cur_c->domain ? cur_c->domain : "",
			.path = cur_c->path ? cur_c->path : "; Path=/",	 // Defalut path is root
			.secure = cur_c->secure ? "; Secure" : "",
			.httponly = cur_c->httponly ? "; HttpOnly" : "",
		};
		fprintf(stderr, "%s=%s%s%s%s%s%s%s\n", t.key, t.value, t.expires, t.max_age, t.domain,
				t.path, t.secure, t.httponly);
		entry = entry->next;
	}
}