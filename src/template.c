#include "template.h"
#include "basic_string.h"
#include "hash_map.h"
#include "utility.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct Template {
    const char *filename;
    Hash_map *context;
};

static inline Basic_string *readfile(const char *filename);

Template *templateNew(const char *filename)
{
    Template *template = malloc(sizeof(Template));

    template->filename = strdup(filename);
    template->context = Hash_map_new();

    return template;
}

void templateDel(Template *template)
{
    free((char *) template->filename);
    Hash_map_delete(template->context);

    free(template);
}

// FIXME: More robust here
void templateSet(Template *template, const char *key, const char *value)
{
    SPair *p = malloc(sizeof(SPair));
    p->key = strdup(key);
    p->value = strdup(value);

    Hash_map_insert_move(template->context, p);
}

Basic_string *templateRender(const Template *template)
{
    Basic_string *res = Basic_string_new();

    Basic_string *buff = readfile(template->filename);
    if (unlikely(!buff))
        goto ret;

    char *pos = NULL;

    // Segment is frozen ctx before {
    char *segment = strtok_r(buff->data, "{", &pos);
    Basic_string_append_raw(res, segment + 1);

    bool rep = false;
    for (;;) {
        // include or placeholder
        // segment's end is '}'
        segment = strtok_r(NULL, "}\0", &pos);

        if (!segment)
            break;

        if (*segment == '{') {  // Is placeholder
            rep = true;
            segment += 1;
            char *val = Hash_map_get(template->context, segment);
            if (val)
                Basic_string_append_raw(res, val);
        } else if (*segment == '%') {
            rep = true;
            segment += 1;
            if (!strncmp(segment, "include", 7)) {  // Is nested include
                segment += 8;
                segment[strlen(segment) - 1] = '\0';  // set '}' to 0

                Template *inc = templateNew(segment);
                Hash_map_delete(inc->context);
                inc->context =
                    template->context;  // !Share my hash_map to nested include

                Basic_string *nested_ctx = templateRender(inc);
                Basic_string_append(res, nested_ctx);
                Basic_string_delete(nested_ctx);

                // nested include's hash_map is also my hash_map, don't delete
                // it
                free((char *)
                         inc->filename);  // Delete nested include's filename
                free(inc);                // Delete nested include
            } else if (!strncmp(segment, "when", 4)) {  // Is status switcher
                segment += 5;
                char *spc = strchr(segment, ' ');
                *spc = '\0';
                char *status_value = Hash_map_get(template->context, segment);

                if (status_value) {     // Found
                    segment = spc + 1;  // spc is the space between status key
                                        // and status value spc + 1 = value
                    spc = strchr(segment, ' ');
                    *spc = '\0';

                    if (!strcmp(status_value, segment)) {
                        segment =
                            spc +
                            1;  // If key is matched, append the action_value
                        segment[strlen(segment) - 1] = '\0';

                        Basic_string_append_raw(res, segment);
                    }
                }
            } else {
                fprintf(stderr, "error: unknown exp {%%%s} in '%s'\n", segment,
                        template->filename);
                goto ret;
            }
        } else {  // Neither placeholder, status switcher nor nest include
            rep = false;
            Basic_string_append_raw(res, "{");
        }

        // Next begin
        segment = strtok_r(NULL, "{\0", &pos);

        if (!segment)
            break;

        if (rep) {
            rep = false;
            segment += 1;
        } else {
            Basic_string_append_raw(res, "}");
        }

        Basic_string_append_raw(res, segment);
    }

ret:
    Basic_string_delete(buff);
    return res;
}

static inline Basic_string *readfile(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (unlikely(!file)) {
        fprintf(stderr, "error: template '%s' not found\n", filename);
        fclose(file);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    rewind(file);

    Basic_string *s = malloc(sizeof(Basic_string));

    s->data = malloc(len + 2);
    fread(s->data + 1, sizeof(char), len, file);
    fclose(file);

    s->size = len;
    s->data[0] = ' ';
    s->data[len] = 0;

    return s;
}