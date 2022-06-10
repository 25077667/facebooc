#ifndef MODELS_ACCOUNT_H
#define MODELS_ACCOUNT_H

#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>

#include "http/cookies.h"
#include "list.h"

typedef struct Account {
	int32_t id;
	int createdAt;

	char* name;
	char* email;
	char* username;
} Account;

// Account ctor
Account* accountNew(int id, int create_at, const char* name, const char* email,
					const char* username);

// Create a new account into db
Account* accountCreate(sqlite3* db, const char* name, const char* email, const char* username,
					   const char* password);
// Get sid string from cookies
const Account* get_account(const Cookies* c);
// Get account by uid, return NULL if not found
Account* accountGetById(sqlite3* db, int uid);
// Get account by email, return NULL if not found
Account* accountGetByEmail(sqlite3* db, const char* email);
// Get account by sid, return NULL if not found
Account* accountGetBySId(sqlite3* db, const char* sid);
// Find name or emmail or username from db
//
// Detail: @page is the OFFSET you had viewed, because of the performance issue
// 		   we search a page (10 entries) once.
Node* accountSearch(sqlite3* db, const char* what_to_search, int page);
// Check if username is taken
bool accountCheckUsername(sqlite3* db, const char* username);
// Check if email is taken
bool accountCheckEmail(sqlite3* db, const char* email);
// Return true if authentication successful, false o.w.
bool account_auth(sqlite3* db, const char* username, const char* password);
// Account dtor, pass NULL is safe
void accountDel(Account* acc);

#endif
