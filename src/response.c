#include "response.h"
#include "http/header.h"
#include "utility.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static const char *STATUSES[6][25] = {
    {""},
    {"Continue", "Switching Protocols"},
    {"OK", "Created", "Accepted", "Non-Authoritative Information", "No Content",
     "Reset Content", "Partial Content"},
    {"Multiple Choices", "Moved Permanently", "Found", "See Other",
     "Not Modified", "Use Proxy", "Switch Proxy", "Temporary Redirect",
     "Permanent Redirect"},
    {"Bad Request",
     "Unauthorized",
     "Payment Required",
     "Forbidden",
     "Not Found",
     "Method Not Allowed",
     "Not Acceptable",
     "Proxy Authentication Required",
     "Request Timeout",
     "Conflict",
     "Gone",
     "Length Required",
     "Precondition Failed",
     "Request Entity Too Large",
     "Request-URI Too Long",
     "Unsupported Media Type",
     "Requested Range Not Satisfiable",
     "Expectation Failed",
     "I'm A Teapot",
     "Authentication Timeout",
     "Enhance Your Calm"},
    {"Internal Server Error", "Not Implemented", "Bad Gateway",
     "Service Unavailable", "Gateway Timeout", "HTTP Version Not Supported"},
};

// Refer to https://datatracker.ietf.org/doc/html/rfc2616
// Section 6
struct Response {
    Status status;
    Header *header;
    size_t body_len;
    const char *body;
};

Response *responseNew()
{
    Response *response = malloc(sizeof(Response));
    response->status = OK;
    response->header = header_new();
    response->body_len = 0;
    response->body = NULL;
    return response;
}

Response *responseNewRedirect(const char *location)
{
    Response *response = responseNew();
    const SSPair p = {.key = "Location", .value = (char *) location};

    responseSetStatus(response, FOUND);
    responseAddHeader(response, &p);

    return response;
}

void responseSetStatus(Response *response, Status status)
{
    response->status = status;
}

void responseSetBody(Response *restrict r, const Basic_string *restrict ctx)
{
    free((char *) r->body);
    r->body = memdup(ctx->data, ctx->size);
    r->body_len = ctx->size;
}

void responseSetBody_move(Response *restrict response,
                          Basic_string *restrict ctx)
{
    free((char *) response->body);
    response->body = ctx->data;
    response->body_len = ctx->size;
    free(ctx);
    ctx = NULL;
}

int response_get_status(const Response *r)
{
    if (unlikely(!r))
        return BAD_REQUEST;

    return r->status;
}

void responseAddCookie(Response *restrict r, const Cookie *restrict c)
{
    SSPair *p_ = malloc(sizeof(SSPair));
    p_->key = strdup("Set-Cookie");
    p_->value = Cookie_to_string(c);

    responseAddHeader_move(r, p_);
}

void responseAddHeader(Response *restrict r, const SSPair *restrict p)
{
    header_insert(r->header, p);
}

void responseAddHeader_move(Response *restrict r, SSPair *restrict p)
{
    header_insert_move(r->header, p);
    p = NULL;
}

void responseDel(Response *response)
{
    if (unlikely(!response))
        return;

    header_delete(response->header);
    free((void *) response->body);
    free(response);
    response = NULL;
}

//   RFC 2616, Section 6 Response
//   Response            = Status-Line              ; Section 6.1
//                        *(( general-header        ; Section 4.5
//                         | response-header        ; Section 6.2
//                         | entity-header ) CRLF)  ; Section 7.1
//                        CRLF
//                        [ message-body ]          ; Section 7.2
void responseWrite(const Response *r, int fd)
{
    if (unlikely(!r))
        return;

    // Send Status-Line
    char status_line[128];
    snprintf(status_line, 128, "HTTP/1.0 %d %s\r\n", r->status,
             STATUSES[r->status / 100][r->status % 100]);
    send(fd, status_line, strlen(status_line), 0);

    // Send header
    char *header = header_to_string(r->header);
    if (header)
        send(fd, header, strlen(header), 0);
    free(header);

    // Header and body seperator
    send(fd, "\r\n", 2, 0);

    // Send body
    send(fd, r->body, r->body_len, 0);
}
