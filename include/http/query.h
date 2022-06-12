#ifndef __QUERY_H__
#define __QUERY_H__

typedef struct Query Query;

// Suppose it is end by '\0'
Query* query_parser(char* path);
void* query_get(const Query* restrict q, const char* restrict key);
void query_delete(Query* q);

#endif