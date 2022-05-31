#ifndef __UTILITY_H__
#define __UTILITY_H__
#include <stddef.h>
#include <stdlib.h>

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

#ifndef container_of_p
#ifdef __GNUC__
#define container_of_p(ptr, type, ptr_member)                       \
	__extension__({                                                 \
		const __typeof__(((type*)0)->ptr_member) __pmember = (ptr); \
		(type*)((char*)__pmember - offsetof(type, ptr_member));     \
	})
#else
#define container_of_p(ptr, type, ptr_member) ((type*)((char*)(ptr)-offsetof(type, ptr_member)))
#endif
#endif

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// Using __typeof__ extension by GCC
// ! Be careful, the type of value might not be deduced as you expect
#define CONST_INIT(x, value) *(__typeof__(value)*)(&x) = value

int string_hash(const char* s);
int obj_hash(const void* data, size_t size);
// trigger the address sanitizer
void mem_canary_alert(const char* msg);

void* memdup(const void* mem, size_t size);

// Copy "^%s/.*$" which %s from src to dst
// %s will not contain any '/'
void fetch_dir(char* restrict dst, const char* restrict src);

#endif