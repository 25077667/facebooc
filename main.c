#include <signal.h>
#include <stdio.h>

#include "facebooc_server.h"

static void sig(int signum);
Facebooc *fb;

static inline _Bool is_number(const char *str)
{
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9')
            return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    if (signal(SIGINT, sig) == SIG_ERR || signal(SIGTERM, sig) == SIG_ERR) {
        fprintf(stderr, "error: failed to bind signal handler\n");
        return 1;
    }

    uint16_t server_port = 8080;

    // parse argv[1] as server port
    if (argc > 1) {
        if (!is_number(argv[1])) {
            fprintf(stderr, "error: invalid port number\n");
            return 1;
        }
        server_port = atoi(argv[1]);
    }

    fb = FB_new(server_port);
    int res = FB_run(fb);
    if (res) {
        // Do some error handling
        fprintf(stderr, "Stop fb server.\n");
    }
    FB_delete(fb);

    return 0;
}

static void sig(int signum)
{
    FB_delete(fb);

    fprintf(stdout, "\n[%d] Bye!\n", signum);
    exit(0);
}