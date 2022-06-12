#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http/http.h"
#include "request.h"
#include "utility.h"

static inline _Bool set_method(Request* req, const char* segment) {
	static const char* methods__[] = {
		"OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE", "CONNECT",
	};
	const int len = sizeof(methods__) / sizeof(*methods__);

	for(int i = 0; i < len; i++)
		if(strcmp(segment, methods__[i]) == 0) {
			CONST_INIT(req->method, i);
			return true;
		}

	CONST_INIT(req->method, UNKNOWN_METHOD);
	return false;
}

static inline Request* request_ctor() {
	Request* request = malloc(sizeof(Request));
	assert(request);
	CONST_INIT(request->method, UNKNOWN_METHOD);
	request->path = NULL;
	request->uri = NULL;
	request->queryString = NULL;
	request->postBody = NULL;
	request->headers = NULL;
	request->cookies = NULL;
	return request;
}

static inline size_t get_uri_len(const char* buf) {
	size_t cur = -1;

	while(buf[++cur] != ' ')
		;

	// Start the uri
	size_t counter = 0;
	while(buf[++cur] != ' ')  // end of uri is a space
		++counter;

	return counter;
}

// Parse Request line, rfc2616 section 5.1
// Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
static inline _Bool parse_request_line(Request* req, const char* buf, size_t* offset) {
	// Suppose buf is not empty (Remove un-essential branch)

	// Get uri len
	const size_t uri_len = get_uri_len(buf);

	char method[8];
	char* uri = malloc(uri_len + 1);
	char version[9];
	sscanf(buf, "%s \t %s \t %s \r\n", method, uri, version);
	uri[uri_len] = 0;

	if(set_method(req, method) == false)
		goto fail;

	// Check version string
	if(strncmp("HTTP/1.", version, 6))
		goto fail;
	if(version[7] != '1' && version[7] != '0')
		goto fail;

	// Actual init uri and path
	CONST_INIT(req->path, uri);
	CONST_INIT(req->uri, uri);
	*offset = strlen(method) + uri_len + strlen(version)
			  + strlen(" "
					   " \r\n");

	return true;

fail:
	free(uri);
	return false;
}

// Return true for success
// If no query, always success
static inline _Bool split_query_string(Request* req) {
	char* begin = strchr(req->path, '?');
	if(!begin)	// No query
		return true;

	const size_t new_uri_len = begin - req->path;
	req->uri = malloc(new_uri_len + 1);	 // split an uri from path
	memcpy((char*)req->uri, req->path, new_uri_len);
	CONST_INIT(req->uri[new_uri_len], (char)0);

	req->queryString = query_parser(begin + 1);

	return !!req->queryString;
}

// Parse HTTP request
// Refer to https://datatracker.ietf.org/doc/html/rfc2616
// generic-message: rfc2616 section 4.1
// generic-message = Request-Line
//                   *(message-header CRLF)
//                   *CRLF
//                   [ message-body ]
Request* requestNew(char* buff) {
	Request* request = request_ctor();

	size_t offset = 0;
	if(parse_request_line(request, buff, &offset) == false)
		goto fail;

	// QUERYSTRING
	if(split_query_string(request) == false)
		goto fail;

	// HEADERS, Read until "\r\n"
	request->headers = header_parser(buff + offset, &offset);
	// COOKIES
	request->cookies = cookies_parser(request->headers);
	// BODY
	request->postBody = body_parser(request->headers, buff + offset);

	return request;

fail:
	requestDel(request);

	return NULL;
}

void requestDel(Request* req) {
	if(req->path != req->uri)
		free((char*)req->uri);	// It has a query, uri is splitted from path

	free((char*)req->path);

	query_delete((Query*)req->queryString);

	body_delete((Body*)req->postBody);

	header_delete((Header*)req->headers);

	Cookies_delete((Cookies*)req->cookies);

	free(req);
}
