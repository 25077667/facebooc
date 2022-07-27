#ifndef __HELPER_HTTP_H__
#define __HELPER_HTTP_H__

#include "pair.h"
#include "string_view.h"

#include <stddef.h>

// Would convert str to decoded url format
// This will reserve some more spaces to make parser easier to put identifier at the end
// The reserved space might holding 16 char
// And, this function also constrain the end of decoded string is NULL byte
char* url_decoder(const char* str);

// New a pair which duplicates the key-value pair like "<key>=<value>"
// <key> is separated at the 'first' appearing '=' character
// <value> is start at the one byte next to the fist '=', and end at the ena of string
SPair* query_entry(const char* str);

// New the first pair which matches this pattern (suppose delim='=', term='&'):
// "^\([^=]*\)=\([^&]*\)&" in regex, \1 is ley, \2 is value
// Return NULL if it not found any pattern
SPair* pair_lexer(const char* str, char delim, char term);

#endif