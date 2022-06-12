#include "db.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static struct sqlite3* DB;
static void createDB(sqlite3* DB, const char* e);

void initDB() {
	const char* db_path = getenv("DB_PATH");
	if(!db_path) {
		struct stat buf;
		if(stat("./data", &buf) == -1)
			mkdir("./data", 0700);
		db_path = "./data/db.sqlite3";
	}
	if(sqlite3_open(db_path, &DB)) {
		fprintf(stderr, "error: unable to open DB: %s\n", sqlite3_errmsg(DB));
		exit(1);
	}

	createDB(DB, "CREATE TABLE IF NOT EXISTS accounts ("
				 "  id        INTEGER PRIMARY KEY ASC"
				 ", createdAt INTEGER"
				 ", name      TEXT"
				 ", username  TEXT"
				 ", email     TEXT UNIQUE"
				 ", password  TEXT"
				 ")");

	createDB(DB, "CREATE TABLE IF NOT EXISTS sessions ("
				 "  id        INTEGER PRIMARY KEY ASC"
				 ", createdAt INTEGER"
				 ", account   INTEGER"
				 ", session   TEXT"
				 ")");

	createDB(DB, "CREATE TABLE IF NOT EXISTS connections ("
				 "  id        INTEGER PRIMARY KEY ASC"
				 ", createdAt INTEGER"
				 ", account1  INTEGER"
				 ", account2  INTEGER"
				 ")");

	createDB(DB, "CREATE TABLE IF NOT EXISTS posts ("
				 "  id        INTEGER PRIMARY KEY ASC"
				 ", createdAt INTEGER"
				 ", author    INTEGER"
				 ", body      TEXT"
				 ")");

	createDB(DB, "CREATE TABLE IF NOT EXISTS likes ("
				 "  id        INTEGER PRIMARY KEY ASC"
				 ", createdAt INTEGER"
				 ", account   INTEGER"
				 ", author    INTEGER"
				 ", post      INTEGER"
				 ")");
}

static void createDB(sqlite3* DB, const char* e) {
	char* err;

	if(sqlite3_exec(DB, e, NULL, NULL, &err) != SQLITE_OK) {
		fprintf(stderr, "error: initDB: %s\n", err);
		sqlite3_free(err);
		exit(1);
	}
}

sqlite3* get_db(void) {
	return DB;
}

void db_close() {
	if(DB) {
		sqlite3_db_cacheflush(DB);
		sqlite3_close(DB);
	}
}
