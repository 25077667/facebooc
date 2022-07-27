#include <time.h>

#include "bs.h"
#include "models/post.h"

static inline Posts* Posts_new(void);

Post* postNew(int id, int createdAt, int authorId, const char* body) {
	Post* post = malloc(sizeof(Post));

	post->id = id;
	post->createdAt = createdAt;
	post->authorId = authorId;
	post->body = bsNew(body);

	return post;
}

Post* postCreate(sqlite3* DB, int authorId, const char* body) {
	int rc, t;
	Post* post = NULL;
	sqlite3_stmt* statement;

	t = time(NULL);
	rc = sqlite3_prepare_v2(DB,
							"INSERT INTO posts(createdAt, author, body)"
							"     VALUES      (        ?,      ?,    ?)",
							-1, &statement, NULL);

	if(rc != SQLITE_OK)
		return NULL;

	char* escapedBody = bsEscape(body);

	if(sqlite3_bind_int(statement, 1, t) != SQLITE_OK)
		goto fail;
	if(sqlite3_bind_int(statement, 2, authorId) != SQLITE_OK)
		goto fail;
	if(sqlite3_bind_text(statement, 3, escapedBody, -1, NULL) != SQLITE_OK)
		goto fail;

	if(sqlite3_step(statement) == SQLITE_DONE)
		post = postNew(sqlite3_last_insert_rowid(DB), t, authorId, body);

fail:
	bsDel(escapedBody);
	sqlite3_finalize(statement);
	return post;
}

Post* postGetById(sqlite3* DB, int id) {
	if(id == -1)
		return NULL;

	Post* post = NULL;
	sqlite3_stmt* statement;

	if(sqlite3_prepare_v2(DB,
						  "SELECT id, createdAt, author, body"
						  "  FROM posts"
						  " WHERE id = ?",
						  -1, &statement, NULL)
	   != SQLITE_OK) {
		return NULL;
	}

	if(sqlite3_bind_int(statement, 1, id) != SQLITE_OK)
		goto fail;
	if(sqlite3_step(statement) != SQLITE_ROW)
		goto fail;

	post = postNew(sqlite3_column_int(statement, 0), sqlite3_column_int(statement, 1),
				   sqlite3_column_int(statement, 2),
				   bsNewline2BR((char*)sqlite3_column_text(statement, 3)));

fail:
	sqlite3_finalize(statement);
	return post;
}

Posts* postGetLatest(sqlite3* DB, int accountId, int page) {
	if(accountId == -1)
		return NULL;

	Posts* ps = Posts_new();
	sqlite3_stmt* statement = NULL;

	int rc = sqlite3_prepare_v2(DB,
								"SELECT id, createdAt, author, body"
								"  FROM posts"
								" WHERE author = ?"
								" ORDER BY createdAt DESC"
								" LIMIT 10 "
								"OFFSET ?",
								-1, &statement, NULL);

	if(rc != SQLITE_OK)
		return NULL;
	if(sqlite3_bind_int(statement, 1, accountId) != SQLITE_OK)
		goto fail;
	if(sqlite3_bind_int(statement, 2, page * 10) != SQLITE_OK)
		goto fail;

	while(sqlite3_step(statement) == SQLITE_ROW) {
		Posts* new_ps = Posts_new();
		new_ps->p = postNew(sqlite3_column_int(statement, 0), sqlite3_column_int(statement, 1),
							sqlite3_column_int(statement, 2),
							bsNewline2BR((char*)sqlite3_column_text(statement, 3)));

		List_insert_tail(&ps->list, &new_ps->list);
	}

fail:
	sqlite3_finalize(statement);
	return ps;
}

Posts* postGetLatestGraph(sqlite3* DB, int accountId, int page) {
	if(accountId == -1)
		return NULL;

	Posts* ps = Posts_new();
	sqlite3_stmt* statement = NULL;

	int rc = sqlite3_prepare_v2(DB,
								"SELECT posts.id, posts.createdAt, posts.author, posts.body"
								"  FROM posts"
								"  LEFT OUTER JOIN connections"
								"    ON posts.author = connections.account2"
								" WHERE connections.account1 = ?"
								"    OR posts.author = ?"
								" ORDER BY posts.createdAt DESC"
								" LIMIT 10 "
								"OFFSET ?",
								-1, &statement, NULL);

	if(rc != SQLITE_OK)
		return NULL;
	if(sqlite3_bind_int(statement, 1, accountId) != SQLITE_OK)
		goto fail;
	if(sqlite3_bind_int(statement, 2, accountId) != SQLITE_OK)
		goto fail;
	if(sqlite3_bind_int(statement, 3, page * 10) != SQLITE_OK)
		goto fail;

	while(sqlite3_step(statement) == SQLITE_ROW) {
		Posts* new_ps = Posts_new();
		new_ps->p = postNew(sqlite3_column_int(statement, 0), sqlite3_column_int(statement, 1),
							sqlite3_column_int(statement, 2),
							bsNewline2BR((char*)sqlite3_column_text(statement, 3)));

		List_insert_tail(&ps->list, &new_ps->list);
	}

fail:
	sqlite3_finalize(statement);
	return ps;
}

void postDel(Post* post) {
	if(!post)
		return;

	bsDel(post->body);
	free(post);
	post = NULL;
}

static inline Posts* Posts_new(void) {
	Posts* ps = malloc(sizeof(Posts));
	ps->p = NULL;

	List_ctor(&ps->list);

	return ps;
}

void Posts_delete(Posts* ps) {
	if(unlikely(!ps))
		return;

	List *node = NULL, *safe = NULL;
	list_for_each_safe(node, safe, &ps->list) {
		Posts* ps_node = container_of(node, Posts, list);
		postDel(ps_node->p);
		free(ps_node);
	}

	// Delete head, which is an empty node
	postDel(ps->p);
	free(ps);
	ps = NULL;
}

int Posts_is_empty(const Posts* ps) {
	return List_is_empty(&ps->list);
}