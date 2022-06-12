#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "list.h"

typedef struct Template Template;

Template* templateNew(const char* filename);
void templateDel(Template* t);
void templateSet(Template* template, const char* key, const char* value);
char* templateRender(const Template* t);

#endif
