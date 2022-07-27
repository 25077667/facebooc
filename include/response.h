#ifndef RESPONSE_H
#define RESPONSE_H

#include "http/cookies.h"
#include "list.h"

typedef enum Status {
	CONTINUE = 100,
	SWITCHING_PROTOCOLS,
	OK = 200,
	CREATED,
	ACCEPTED,
	NAI,
	NO_CONTENT,
	RESET_CONTENT,
	PARTIAL_CONTENT,
	MULTIPLE_CHOICES = 300,
	MOVED_PERMANENTLY,
	FOUND,
	SEE_OTHER,
	NOT_MODIFIED,
	USE_PROXY,
	SWITCH_PROXY,
	TEMPORARY_REDIRECT,
	PERMANENT_REDIRECT,
	BAD_REQUEST = 400,
	UNAUTHORIZED,
	PAYMENT_REQUIRED,
	FORBIDDEN,
	NOT_FOUND,
	METHOD_NOT_ALLOWED,
	NOT_ACCEPTABLE,
	PROXY_AUTH_REQUIRED,
	REQUEST_TIMEOUT,
	CONFLICT,
	GONE,
	LENGTH_REQUIRED,
	PRECONDITION_FAILED,
	REQUEST_ENTITY_TOO_LARGE,
	REQUEST_URI_TOO_LONG,
	UNSUPPORTED_MEDIA_TYPE,
	REQUESTED_RANGE_NOT_SATISFIABLE,
	EXPECTATION_FAILED,
	IM_A_TEAPOT,
	AUTHENTICATION_TIMEOUT,
	ENHANCE_YOUR_CALM,
	INTERNAL_SERVER_ERROR = 500,
	NOT_IMPLEMENTED,
	BAD_GATEWAY,
	SERVICE_UNAVAILABLE,
	GATEWAY_TIMEOUT,
	HTTP_VERSION_NOT_SUPPORTED
} Status;

typedef struct Response Response;

Response* responseNew();
Response* responseNewRedirect(const char* location);
void responseSetStatus(Response* r, Status);

// ! This ctx is bs
void responseSetBody_move(Response* restrict r, const char* restrict ctx);

void responseSetBody_data(Response* restrict r, const void* restrict ctx, size_t len);
void responseSetBody_data_move(Response* restrict r, void* restrict ctx, size_t len);

int response_get_status(const Response* r);

void responseAddCookie(Response* restrict r, const Cookie* restrict cookie_entry);
void responseAddHeader(Response* restrict r, const SSPair* restrict p);
void responseAddHeader_move(Response* restrict r, SSPair* restrict p);
void responseDel(Response* r);

void responseWrite(const Response* r, int fd);

// TODO: splice, sendfile supporting
#endif
