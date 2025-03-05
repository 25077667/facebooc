#include "basic_string.h"
#include <stdlib.h>
#include <string.h>
#include "utility.h"  // Assumes unlikely() is defined here.

Basic_string *Basic_string_new(void)
{
    Basic_string *s = malloc(sizeof(Basic_string));
    if (!s)
        return NULL;
    s->data = strdup("");
    if (!s->data) {
        free(s);
        return NULL;
    }
    s->size = 0;
    return s;
}

Basic_string *Basic_string_init(const char *src)
{
    if (unlikely(!src))
        return Basic_string_new();

    Basic_string *s = malloc(sizeof(Basic_string));
    if (!s)
        return NULL;
    s->data = strdup(src);
    if (!s->data) {
        free(s);
        return NULL;
    }
    s->size = strlen(src);
    return s;
}

Basic_string *Basic_string_combine(const Basic_string *s1,
                                   const Basic_string *s2)
{
    if (unlikely(!s1 || !s2))
        return NULL;

    Basic_string *s = malloc(sizeof(Basic_string));
    if (!s)
        return NULL;

    size_t new_size = s1->size + s2->size;
    s->data = malloc(new_size + 1);
    if (!s->data) {
        free(s);
        return NULL;
    }

    memcpy(s->data, s1->data, s1->size);
    memcpy(s->data + s1->size, s2->data, s2->size);
    s->data[new_size] = '\0';
    s->size = new_size;
    return s;
}

void Basic_string_append(Basic_string *dst, const Basic_string *src)
{
    if (unlikely(!dst || !src))
        return;

    size_t new_size = dst->size + src->size;
    char *tmp = realloc(dst->data, new_size + 1);
    if (!tmp)
        return;  // Leave dst unmodified if allocation fails.
    dst->data = tmp;

    memcpy(dst->data + dst->size, src->data, src->size);
    dst->size = new_size;
    dst->data[new_size] = '\0';
}

void Basic_string_append_raw(Basic_string *dst, const char *src)
{
    if (unlikely(!dst || !dst->data || !src))
        return;

    size_t len = strlen(src);
    if (len == 0)
        return;

    size_t new_size = dst->size + len;
    char *tmp = realloc(dst->data, new_size + 1);
    if (!tmp)
        return;  // Leave dst unchanged on allocation failure.
    dst->data = tmp;

    memcpy(dst->data + dst->size, src, len);
    dst->size = new_size;
    dst->data[new_size] = '\0';
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
    /* Setting the local pointer to NULL does not affect the caller. */
}

void Basic_string_delete_const(const Basic_string *str)
{
    Basic_string_delete((Basic_string *) str);
}
