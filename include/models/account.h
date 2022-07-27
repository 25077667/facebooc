#ifndef MODELS_ACCOUNT_H
#define MODELS_ACCOUNT_H

#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>

#include "list.h"

typedef struct Account {
	int32_t id;
	int createdAt;

	char* name;
	char* email;
	char* username;
} Account;

typedef struct Accounts {
	Account* a;
	List list;
} Accounts;

// Account ctor
Account* accountNew(int id, int create_at, const char* name, const char* email,
					const char* username);

// Create a new account into db
Account* accountCreate(sqlite3* db, const char* name, const char* email, const char* username,
					   const char* password);
// Get account by uid, return NULL if not found
Account* accountGetById(sqlite3* db, int uid);
// Get account by email, return NULL if not found
Account* accountGetByEmail(sqlite3* db, const char* email);
// Get account by sid, return NULL if not found
Account* accountGetBySId(sqlite3* db, const char* sid);
// Find name or email or username from db
//
// Detail: @page is the OFFSET you had viewed, because of the performance issue
// 		   we search a page (10 entries) once.
Accounts* accountSearch(sqlite3* db, const char* what_to_search, int page);
// Check if username is taken
bool accountCheckUsername(sqlite3* db, const char* username);
// Check if email is taken
bool accountCheckEmail(sqlite3* db, const char* email);
// Return true if authentication successful, false o.w.
bool account_auth(sqlite3* db, const char* username, const char* password);
// Account dtor, pass NULL is safe
void accountDel(Account* acc);

void accounts_delete(Accounts* as);

int accounts_is_empty(const Accounts* as);

#endif
