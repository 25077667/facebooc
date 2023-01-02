#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
typedef int sockopt_t;

#include "db.h"
#include "server.h"
#include "utility.h"

static char *METHODS[8] = {"OPTIONS", "GET",    "HEAD",  "POST",
                           "PUT",     "DELETE", "TRACE", "CONNECT"};

static const int EPOLL_NUM = 64;

static inline void pop_log(const struct sockaddr_in *addr,
                           const char *method,
                           const char *path,
                           int status)
{
    time_t t = time(NULL);
    char timebuff[100];
    strftime(timebuff, sizeof(timebuff), "%c", localtime(&t));
    fprintf(stderr, "%s %s %s %s %d\n", timebuff, inet_ntoa(addr->sin_addr),
            method, path, status);
}

Server *serverNew(const uint16_t port)
{
    Server *server = malloc(sizeof(Server));
    server->port = port;
    server->priv = epoll_create1(0);
    server->handlers = Hash_map_new();
    return server;
}

void serverDel(Server *server)
{
    Hash_map_delete(server->handlers);
    free(server);
}

void serverAddHandler(Server *server, const char *route_name, Handler handler)
{
    SPair *new_handler = malloc(sizeof(SPair));
    new_handler->key = strdup(route_name);
    new_handler->value = memdup(&handler, sizeof(Handler));

    Hash_map_insert_move(server->handlers, new_handler);
}

void set_callback(Server *server, Handler handler)
{
    server->default_callback = handler;
}

static inline int makeSocket(unsigned int port)
{
    const int sock = socket(PF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        fprintf(stderr, "error: failed to create socket\n");
        exit(1);
    }

    sockopt_t optval = 1; /* prevent from address being taken */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    const struct sockaddr_in addr = {.sin_family = AF_INET,
                                     .sin_port = htons(port),
                                     .sin_addr.s_addr = htons(INADDR_ANY)};

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        fprintf(stderr, "error: failed to bind socket to 0.0.0.0:%u \n", port);
        exit(1);
    }

    if (listen(sock, 1) < 0) {
        fprintf(stderr, "error: socket failed to listen\n");
        exit(1);
    }

    return sock;
}

static void setFdNonblocking(int fd)
{
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) < 0)
        perror("fcntl");
}

static void serverAddFd(int epollfd, int fd, int in, _Bool oneshot)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLET | EPOLLRDHUP;
    event.events |= in ? EPOLLIN : EPOLLOUT;
    if (oneshot)
        event.events |= EPOLLONESHOT;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) < 0)
        perror("epoll_ctl");
    setFdNonblocking(fd);
}

static void resetOneShot(int epollfd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event) < 0)
        perror("epoll_ctl");
}

static void serverDelFd(Server *server, int fd)
{
    if (epoll_ctl(server->priv, EPOLL_CTL_DEL, fd, NULL) < 0)
        perror("epoll_ctl");
}

static inline void handle(Server *server, int fd, struct sockaddr_in *addr)
{
    int nread = 0;
    char buff[20480];

    if ((nread = recv(fd, buff, sizeof(buff), 0)) < 0) {
        if (errno == EAGAIN) {
            resetOneShot(server->priv, fd);
        } else {
            fprintf(stderr, "error: read failed\n");
        }
    } else if (nread > 0) {
        buff[nread] = '\0';

        Request *req = requestNew(buff);

        if (!req) {
            send(fd, "HTTP/1.0 400 Bad Request\r\n\r\nBad Request", 39, 0);
            pop_log(addr, "", "", 400);
        } else {
            // Route one dir in once
            char dir[32] = {0};  // Suppose single dir is less than 32 char
            fetch_dir(dir, req->uri + 1);

            Handler *handler_func = Hash_map_get(server->handlers, dir);
            if (!handler_func)  // not found
                handler_func = &server->default_callback;

            Response *response = (*handler_func)(req);
            if (unlikely(!response))  // Return NULL, goto not found
                response = server->default_callback(req);

            pop_log(addr, METHODS[req->method], req->path,
                    response_get_status(response));

            responseWrite(response, fd);
            responseDel(response);

            requestDel(req);
        }
    }
    serverDelFd(server, fd);
    close(fd);
}

void serverServe(Server *server)
{
    const int sock = makeSocket(server->port);
    struct epoll_event *events = malloc(sizeof(struct epoll_event) * EPOLL_NUM);

    serverAddFd(server->priv, sock, 1, false);

    fprintf(stdout, "Listening on port %d.\n\n", server->port);

    for (;;) {
        const int nfds = epoll_wait(server->priv, events, EPOLL_NUM, -1);
        for (int i = 0; i < nfds; ++i) {
            struct epoll_event event = events[i];
            const int tmpfd = event.data.fd;
            struct sockaddr_in addr;
            if (tmpfd == sock) {
                socklen_t size = sizeof(addr);
                int newSock = 0;
                while ((newSock = accept(sock, (struct sockaddr *) &addr,
                                         &size)) > 0) {
                    serverAddFd(server->priv, newSock, 1, true);
                }
                if (newSock == -1) {
                    if (errno != EAGAIN && errno != ECONNABORTED &&
                        errno != EPROTO && errno != EINTR) {
                        fprintf(stderr, "error: failed to accept connection\n");
                        exit(1);
                    }
                }
            } else {
                handle(server, tmpfd, &addr);
            }
        }
    }
}
