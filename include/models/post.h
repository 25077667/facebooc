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

typedef struct Posts {
	Post* p;
	List list;
} Posts;

Post* postNew(int id, int createdAt, int authorId, const char* body);
Post* postCreate(sqlite3* db, int uid, const char* body);
Post* postGetById(sqlite3* db, int id);
Posts* postGetLatest(sqlite3* db, int authorId, int page);
Posts* postGetLatestGraph(sqlite3*, int authorId, int page);
void postDel(Post* p);

// Posts' destructor
void Posts_delete(Posts* ps);

int Posts_is_empty(const Posts* ps);

#endif
