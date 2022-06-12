#ifndef REQUEST_H
#define REQUEST_H

#include "http/body.h"
#include "http/cookies.h"
#include "http/header.h"
#include "http/query.h"

typedef enum Method {
	OPTIONS,
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	TRACE,
	CONNECT,
	UNKNOWN_METHOD
} Method;

typedef struct Request {
	const Method method;

	const char* path;
	const char* uri;

	const Query* queryString;
	const Body* postBody;
	const Cookies* cookies;
	const Header* headers;
} Request;

Request* requestNew(char*);
void requestDel(Request*);

#endif
