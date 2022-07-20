#ifndef __HELPER_HTTP_H__
#define __HELPER_HTTP_H__

#include "pair.h"
#include "string_view.h"
#include "utility.h"

#include <stddef.h>

// Using __typeof__ extension by GCC
// ! Be careful, the type of value might not be deduced as you expect
#define CONST_INIT(x, value) *(__typeof__(value)*)(&x) = value

char* url_decoder(const char* str);
SPair* query_entry(const char* str);

#endif