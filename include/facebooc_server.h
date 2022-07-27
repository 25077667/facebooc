#ifndef __FACEBOOC_H__
#define __FACEBOOC_H__

#include "server.h"

typedef struct Facebooc Facebooc;

Facebooc* FB_new(const uint16_t port);
int FB_run(Facebooc* s);
void FB_delete(Facebooc* s);

// You should add routing rules in facebooc_server.c

#endif