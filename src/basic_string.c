#include "basic_string.h"
#include "utility.h"

#include <stdlib.h>
#include <string.h>

Basic_string *Basic_string_new()
{
    Basic_string *s = malloc(sizeof(Basic_string));
    s->data = strdup("");
    s->size = 0;
    return s;
}

Basic_string *Basic_string_init(const char *src)
{
    if (unlikely(!src))
        return Basic_string_new();

    Basic_string *s = malloc(sizeof(Basic_string));
    s->data = strdup(src);
    s->size = strlen(src);
    return s;
}

Basic_string *Basic_string_combine(const Basic_string *s1,
                                   const Basic_string *s2)
{
    Basic_string *s = malloc(sizeof(Basic_string));

    s->data = malloc(s1->size + s2->size + 1);
    memcpy(s->data, s1->data, s1->size);
    memcpy(s->data + s1->size, s2->data, s2->size);
    s->data[s1->size + s2->size] = 0;

    s->size = s1->size + s2->size;
    return s;
}

void Basic_string_append(Basic_string *dst, const Basic_string *src)
{
    if (unlikely(!src || !dst))
        return;

    dst->data = realloc(dst->data, dst->size + src->size + 1);

    memcpy(dst->data + dst->size, src->data, src->size);

    dst->size += src->size;
    dst->data[dst->size] = 0;
}

#ifdef Basic_string_delete
#undef Basic_string_delete
#endif

void Basic_string_delete(Basic_string *str)
{
    if (unlikely(!str))
        return;

    free(str->data);
    free(str);
    str = NULL;
}

void Basic_string_delete_const(const Basic_string *str)
{
    Basic_string_delete((Basic_string *) str);
}

void Basic_string_append_raw(Basic_string *dst, const char *src)
{
    if (unlikely(!dst || !dst->data || !src || !*src))
        return;

    const size_t src_len = strlen(src);
    dst->data = realloc(dst->data, dst->size + src_len + 1);
    memcpy(dst->data + dst->size, src, src_len);
    dst->size += src_len;

    dst->data[dst->size] = 0;
}
