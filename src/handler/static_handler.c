#include "static_handler.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

struct Buf_size {
    void *addr;
    size_t len;
};

// compare str1 and str2 from tail
static inline int rev_cmp(const char *str1,
                          size_t len1,
                          const char *str2,
                          size_t len2)
{
    const long smaller = min(len1, len2);
    return strncmp(str1 + len1 - smaller, str2 + len2 - smaller, smaller);
}

static inline const char *mimeType_mux(const char *path)
{
    static const char *file_type[] = {
        "html", "json", "jpeg", "jpg", "gif", "png", "css", "js",
    };
    static const size_t file_type_len = ARR_LEN(file_type);

    static const char *mime_type[] = {
        "text/html",  "application/json",
        "image/jpeg", "image/jpeg",
        "image/gif",  "image/png",
        "text/css",   "application/javascript",
        "text/plain",
    };

    size_t i = 0;
    for (; i < file_type_len; i++) {
        if (rev_cmp(path, strlen(path), file_type[i], strlen(file_type[i])) ==
            0)
            break;
    }

    return mime_type[i];
}

// Each entry of this Hash_map is referenced by mmap
// Don't use free to munmap it
// Also, not expected to munmap it
static Hash_map *static_file_table;

static struct Buf_size get_static_file(const char *filename)
{
    struct Buf_size fm = {.addr = NULL, .len = 0};
    // Check if it is already in the static_file_table
    const struct Buf_size *entry = Hash_map_get(static_file_table, filename);
    if (entry) {  // Found
        fm.addr = entry->addr;
        fm.len = entry->len;
        goto found;
    }

    // EXIT ON DIRS
    struct stat stat_file;
    if (stat(filename, &stat_file) < 0 || S_ISDIR(stat_file.st_mode))
        goto fail;

    // EXIT ON NOT FOUND
    int file = open(filename, O_RDONLY);
    if (unlikely(file == -1)) {
        // Not found or error
        fprintf(stderr, "%s not found\n", filename);
        goto fail;
    };

    // Init local fm
    fm.addr = mmap(NULL, stat_file.st_size, PROT_READ, MAP_PRIVATE, file, 0);
    fm.len = stat_file.st_size;
    madvise(fm.addr, stat_file.st_size,
            MADV_SEQUENTIAL | MADV_WILLNEED | MADV_DONTFORK);
    close(file);

    // Dup a fm into static_file_table
    struct Buf_size *copy_fm = malloc(sizeof(struct Buf_size));
    copy_fm->addr = fm.addr;
    copy_fm->len = fm.len;
    SPair p = {.key = (char *) filename, .value = copy_fm};
    Hash_map_insert_move(static_file_table, &p);

fail:
found:
    return fm;
}

Response *static_handler(const Request *req)
{
    // EXIT ON SHENANIGANS
    if (strstr(req->uri, "../"))
        return NULL;

    const char *filename = req->uri + 1;

    // Prepare static_file_table
    if (unlikely(!static_file_table))
        static_file_table = Hash_map_new();

    struct Buf_size fm = get_static_file(filename);
    if (fm.addr == NULL)  // file not found
        return NULL;

    Response *response = responseNew();
    responseSetBody(
        response,
        (Basic_string *) &fm);  // fm's memory model is same as Basic_string

    // MIME TYPE
    // It is impossible to not found any matched mimetype, because we had check
    // static file is exist
    const char *mimeType = mimeType_mux(req->uri);
    char content_len[11];
    snprintf(content_len, 11, "%lu", fm.len);

    // RESPOND
    responseSetStatus(response, OK);
    responseAddHeader(response,
                      eXpire_pair("Content-Type", (char *) mimeType, SSPair));
    responseAddHeader(response,
                      eXpire_pair("Content-Length", content_len, SSPair));
    responseAddHeader(response,
                      eXpire_pair("Cache-Control", "max-age=2592000", SSPair));
    return response;
}
