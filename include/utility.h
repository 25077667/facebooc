#ifndef __UTILITY_H__
#define __UTILITY_H__
#include "pair.h"
#include "string_view.h"

#include <stddef.h>
#include <stdlib.h>

// Function is a pointer which takes a pointer and returns an int
typedef int (*Func)(void*);

/**
 * container_of() - Calculate address of object that contains address ptr
 * @ptr: pointer to member variable
 * @type: type of the structure containing ptr
 * @member: name of the member variable in struct @type
 *
 * Return: @type pointer of object containing ptr
 */
#ifndef container_of
#ifdef __GNUC__
#define container_of(ptr, type, member)                          \
	__extension__({                                              \
		const __typeof__(((type*)0)->member)* __pmember = (ptr); \
		(type*)((char*)__pmember - offsetof(type, member));      \
	})
#else
#define container_of(ptr, type, member) ((type*)((char*)(ptr)-offsetof(type, member)))
#endif
#endif

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// TODO: Make this mixmax macro more robust
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) < (y) ? (y) : (x))

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))

// Using __typeof__ extension by GCC
// ! Be careful, the type of value might not be deduced as you expect
#define CONST_INIT(x, value) *(__typeof__(value)*)(&x) = value

int string_hash(const char* s);
int obj_hash(const void* data, size_t size);

#ifdef DEBUG
void mem_canary_alert(const char* msg);
#endif

void* memdup(const void* mem, size_t size);

// Copy "^%s/.*$" which %s from src to dst
// %s will not contain any '/'
void fetch_dir(char* restrict dst, const char* restrict src);

void to_lower_case(char* str);

char* find_first_of(const char* str, const char* delim);

SPair* make_pair(const struct string_view* const key, const struct string_view* const value);

#endif