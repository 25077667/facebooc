#ifndef __COOKIES_H__
#define __COOKIES_H__

#include "http/header.h"
#include "pair.h"
#include <stdbool.h>

typedef enum Cookie_attr {
	KEY,	// reserved, don't use
	VALUE,	// reserved, don't use
	EXPIRE,
	MAX_AGE,
	DOMAIN,
	PATH,
	SECURE,
	HTTPONLY,
} Cookie_attr;

// All cookie's element is
// (const char *)(key):(char *)(value)
// A.K.A const string maps to string
typedef struct Cookies Cookies;
typedef struct Cookie Cookie;

// Refers to RFC 6265
// Return a new instance of Cookies structure, you should free it via Cookie_delete
Cookies* cookies_parser(const Header* header);

Cookies* Cookies_new();
// Add a string-void pair to the cookie
// Return the Cookies c
// Will "copy" the p into the Cookies c
Cookies* Cookies_insert(Cookies* c, const char* key, const char* value);

// Equivalent to Cookies_new + Cookies_insert
Cookies* Cookies_init(const char* key, const char* value);

// Use key to search the cookie
// Return NULL if not found
Cookie* Cookies_get(const Cookies* c, const char* key);
void Cookies_delete(Cookies*);

// Overwrite default attribution
void Cookies_set_attr(Cookies* c, const char* key, Cookie_attr attr, const char* attr_value);

char* Cookie_to_string(const Cookies* c);

void Cookie_gen_expire(char dst[64], int duration);

// Equivalent to Cookie_gen_expire + Cookies_set_attr(expire)
void Cookies_set_expire(Cookies* c, const char* key, int dutation);

#define Cookie_get_attr(cookie_p, attr) (((char**)cookie_p)[attr])

#ifdef DEBUG
void Cookies_print(const Cookies* c);
#endif

#endif