#include "handler/connect.h"
#include "handler/dashboard.h"
#include "handler/home.h"
#include "handler/like.h"
#include "handler/login.h"
#include "handler/logout.h"
#include "handler/notFound.h"
#include "handler/post.h"
#include "handler/profile.h"
#include "handler/search.h"
#include "handler/signup.h"
#include "handler/static_handler.h"

#include "db.h"

// You should add routing rules in facebooc_server.c
// Register handler functions
#define register_handler(name_, func_) \
    {                                  \
        .name = name_, .func = func_   \
    }

#define register_handler_same_name(func) register_handler(#func, func)

struct __HANDLER_TABLE__ {
    const char *name;
    Response *((*func)(const Request *) );
} handler_table[] = {
    register_handler_same_name(connect),
    register_handler_same_name(dashboard),
    register_handler("", home),
    register_handler_same_name(like),
    register_handler_same_name(login),
    register_handler_same_name(logout),
    register_handler_same_name(post),
    register_handler_same_name(profile),
    register_handler_same_name(search),
    register_handler_same_name(signup),
    register_handler("static", static_handler),
};

// Facebooc is an directive class of Server
struct Facebooc {
    Server *server;
};


Facebooc *FB_new(const uint16_t port)
{
    Facebooc *s = malloc(sizeof(Facebooc));
    s->server = serverNew(port);

    for (size_t i = 0; i < ARR_LEN(handler_table); i++)
        serverAddHandler(s->server, handler_table[i].name,
                         handler_table[i].func);

    set_callback(s->server, notFound);

    initDB();

    return s;
}

int FB_run(Facebooc *s)
{
    serverServe(s->server);
    return 1;
}

void FB_delete(Facebooc *s)
{
    if (s->server)
        serverDel(s->server);
    void db_close();
    free(s);
    s = NULL;
}
