#ifndef MODELS_POST_H
#define MODELS_POST_H

#include <sqlite3.h>

#include "list.h"

#define MAX_BODY_LEN 65536

typedef struct Post {
	int id;
	int createdAt;
	int authorId;

	char* body;
} Post;

Post* postNew(int, int, int, const char*);
Post* postCreate(sqlite3*, int uid, const char* body);
Post* postGetById(sqlite3*, int);
List* postGetLatest(sqlite3*, int, int);
List* postGetLatestGraph(sqlite3*, int, int);
void postDel(Post*);

#endif
