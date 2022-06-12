#ifndef MODELS_SESSION_H
#define MODELS_SESSION_H

#include <sqlite3.h>

typedef struct Session {
	int id;
	int createdAt;
	int accountId;

	char* sessionId;
} Session;

Session* sessionNew(int, int, int, char*);
Session* sessionGetBySId(sqlite3*, const char*);
Session* sessionCreate(sqlite3*, const char*, const char*);
void sessionDel(Session*);

#endif
