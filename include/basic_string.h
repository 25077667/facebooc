#ifndef __BASIC_STRING_H__
#define __BASIC_STRING_H__
#include <stddef.h>

// A string simulate C++'s basic_string
typedef struct Basic_string {
    char *data;
    size_t size;
} Basic_string;

Basic_string *Basic_string_new(void);
Basic_string *Basic_string_init(const char *src);
Basic_string *Basic_string_combine(const Basic_string *s1,
                                   const Basic_string *s2);
void Basic_string_append(Basic_string *dst, const Basic_string *src);
void Basic_string_append_raw(Basic_string *dst, const char *src);
void Basic_string_delete(Basic_string *str);
void Basic_string_delete_const(const Basic_string *str);

#define Basic_string_delete(str)                         \
    _Generic((str),                                      \
        const Basic_string *: Basic_string_delete_const, \
        Basic_string *: Basic_string_delete)(str)

#endif
