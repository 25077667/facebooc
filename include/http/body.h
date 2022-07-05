#ifndef __BODY_HTTP_H__
#define __BODY_HTTP_H__

#include "header.h"
#include "pair.h"
typedef struct Body Body;

Body* body_parser(const Header* restrict header, char* restrict buffer);

Body* body_new();
Body* body_insert(Body* restrict b, const SPair* restrict p);
Body* body_insert_move(Body* restrict b, SPair* restrict p);
void* body_get(const Body* restrict b, const char* restrict key);
void body_delete(Body* b);

#endif