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
	"",	 // Should be ignored (key)
	"",	 // Should be ignored (value)
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
	Cookie* c;

	List list;
};

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
	if(unlikely(!c))
		return;

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

static Cookie* single_cookie_gen__(const char* str) {
	const char* seperator = find_first_of(str, "=");
	string_view key = { .begin = str, .end = seperator, .size = seperator - str };
	string_view value = { .begin = seperator + 1,
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

Cookies* cookies_parser(const Header* header) {
	if(!header)
		return NULL;
	const char* str = header_get(header, "cookie");
	if(!str)  // cookie is not exist
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

			Cookies* new_cs = Cookies_new();
			new_cs->c = single_cookie_gen__(decoded);
			List_insert_head(&cookies->list, &new_cs->list);

			free(decoded);
		}
		str += strlen(str);
	}

	return cookies;
}

Cookies* Cookies_new(void) {
	Cookies* cs = malloc(sizeof(Cookies));
	cs->c = NULL;
	List_ctor(&cs->list);

	return cs;
}

Cookies* Cookies_insert(Cookies* cs, const char* key, const char* value) {
	if(unlikely(!cs || !key || !*key || !value))  // value could be "" empty string
		return cs;

	Cookie* new_c = single_cookie_new__();
	new_c->key = strdup(key);
	new_c->value = strdup(value);

	Cookies* new_cs = Cookies_new();
	new_cs->c = new_c;
	List_insert_head(&cs->list, &new_cs->list);

	return cs;
}

Cookies* Cookies_init(const char* key, const char* value) {
	return Cookies_insert(Cookies_new(), key, value);
}

Cookie* Cookies_get(const Cookies* c, const char* key) {
	if(unlikely(!c || !key))
		return NULL;

	List* cur = NULL;
	list_for_each(cur, &c->list) {
		Cookies* cs = container_of(cur, Cookies, list);
		if(strcmp(cs->c->key, key) == 0)
			return cs->c;
	}

	return NULL;
}

void Cookies_delete(Cookies* c) {
	if(unlikely(!c))
		return;

	List *node = NULL, *safe = NULL;
	list_for_each_safe(node, safe, &c->list) {
		Cookies* cs_node = container_of(node, Cookies, list);
		single_cookie_delete__(cs_node->c);
		free(cs_node);
	}

	// Delete head, which is an empty node
	free(c);
	c = NULL;
}

Cookie* Cookie_init(const SSPair* p) {
	if(unlikely(!p))
		return NULL;

	SSPair* new_p = malloc(sizeof(SSPair));
	new_p->key = strdup(p->key);
	new_p->value = strdup(p->value);

	return Cookie_init_move(new_p);
}

Cookie* Cookie_init_move(SSPair* p) {
	if(unlikely(!p))
		return NULL;

	Cookie* c = single_cookie_new__();
	c->key = p->key;
	c->value = p->value;
	free(p);
	p = NULL;

	return c;
}

char* Cookie_to_string(const Cookie* c) {
	if(unlikely(!c))
		return NULL;

	// Suppose there is only one cookie,
	const Cookie_to_string_template__ t = {
		.key = c->key,
		.value = c->value,
		.expires = c->expires ? c->expires : "",
		.max_age = c->max_age ? c->max_age : "",
		.domain = c->domain ? c->domain : "",
		.path = c->path ? c->path : "; Path=/",	 // Default path is root
		.secure = c->secure ? "; Secure" : "",
		.httponly = c->httponly ? "; HttpOnly" : "",
	};

	size_t cur_len = cookie_template_len__(&t);

	char* str = malloc(cur_len + 2);  // Add '=' and '\0'

	snprintf(str, cur_len + 2, "%s=%s%s%s%s%s%s%s", t.key, t.value, t.expires, t.max_age, t.domain,
			 t.path, t.secure, t.httponly);

	return str;
}

// RFC 1123
// sane-cookie-date  = <rfc1123-date, defined in [RFC2616], Section 3.3.1>
void Cookie_gen_expire(char* buf, int duration) {
	const time_t t = time(NULL) + duration;
	strftime(buf, 64, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&t));
}

void Cookie_set_attr(Cookie* c, Cookie_attr attr, const char* value) {
	if(unlikely(!c || !value || !*value || attr > HTTPONLY))
		return;

	char** target_p = &((char**)c)[attr];
	free(*target_p);

	const char* fmt_str = c_av_fmt_str[attr];
	size_t len = strlen(fmt_str) + strlen(value) + 1;
	char* buf = malloc(len);
	snprintf(buf, len, fmt_str, value);

	*target_p = buf;
}

#ifdef DEBUG
void Cookies_print(const Cookies* cs) {
	if(unlikely(!cs || List_is_empty(&cs->list)))
		return;

	List* cur = NULL;
	list_for_each(cur, &cs->list) {
		Cookie* cur_c = container_of(cur, Cookies, list)->c;

		const Cookie_to_string_template__ t = {
			.key = cur_c->key,
			.value = cur_c->value,
			.expires = cur_c->expires ? cur_c->expires : "",
			.max_age = cur_c->max_age ? cur_c->max_age : "",
			.domain = cur_c->domain ? cur_c->domain : "",
			.path = cur_c->path ? cur_c->path : "; Path=/",	 // Default path is root
			.secure = cur_c->secure ? "; Secure" : "",
			.httponly = cur_c->httponly ? "; HttpOnly" : "",
		};
		fprintf(stderr, "%s=%s%s%s%s%s%s%s\n", t.key, t.value, t.expires, t.max_age, t.domain,
				t.path, t.secure, t.httponly);
	}
}
#endif

void Cookie_delete(Cookie* c) {
	single_cookie_delete__(c);
}