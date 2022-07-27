#ifndef __HEADER_HTTP_H__
#define __HEADER_HTTP_H__

#include "pair.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct Header Header;

Header* header_parser(char* restrict buffer, size_t* restrict offset);
// ctor an empty header
Header* header_new();
void header_insert(Header* restrict h, const SSPair* restrict p);
void header_insert_move(Header* restrict h, SSPair* restrict p);
const char* header_get(const Header* restrict h, const char* restrict key);
void header_delete(Header* h);

// copy header context to string
char* header_to_string(const Header*);

#ifdef DEBUG
void Header_print(const Header* h);
#endif

#endif