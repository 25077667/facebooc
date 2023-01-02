#include "utility.h"

#include <ctype.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/sha.h>

static inline void badCharHeuristic(const char *str,
                                    size_t size,
                                    char badchar[256]);
static char *boyer_moore(const char *txt, const char *pat);

// Reference: http://www.cse.yorku.ca/~oz/hash.html
int string_hash(const char *str)
{
    int hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

// sdbm
int obj_hash(const void *data, size_t size)
{
    unsigned long hash = 0;

    for (size_t i = 0; i < size; i++) {
        hash = ((int *) data)[i] + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

#ifdef DEBUG
void mem_canary_alert(const char *msg)
{
    puts(msg);
#ifdef __SANITIZE_ADDRESS__
    char *canary = NULL;
    *canary = 0;
#else
    void *buffer[64];
    char **symbols;

    int num = backtrace(buffer, 64);
    printf("backtrace() returned %d addresses\n", num);

    symbols = backtrace_symbols(buffer, num);
    if (symbols == NULL) {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for (int j = 0; j < num; j++)
        printf("%s\n", symbols[j]);

    free(symbols);
#endif
}
#endif

void *memdup(const void *mem, size_t size)
{
    void *out = malloc(size);
    if (out != NULL)
        memcpy(out, mem, size);
    return out;
}

void fetch_dir(char *restrict dst, const char *restrict src)
{
    const char *sep = strchr(src, '/');
    if (sep)
        memcpy(dst, src, sep - src);
}

void to_lower_case(char *str)
{
    while (str && *str) {
        *str = tolower(*str);
        str++;
    }
}

// Return NULL if not found
char *find_first_of(const char *str, const char *delim)
{
    if (str && *str)
        return boyer_moore(str, delim);
    else
        return NULL;
}

static inline void badCharHeuristic(const char *str,
                                    size_t size,
                                    char badchar[256])
{
    memset(badchar, -1, 256);

    for (size_t i = 0; i < size; i++)
        badchar[(int) str[i]] = i;
}

// pat is non-empty
// https://www.geeksforgeeks.org/boyer-moore-algorithm-for-pattern-searching/
// Return NULL if not found
static char *boyer_moore(const char *txt, const char *pat)
{
    size_t m = strlen(pat);
    size_t n = strlen(txt);

    /* Quick edge cases. */
    if (m == 0 || m > n)
        return NULL;
    if (m == 1)
        return strchr(txt, *pat);

    char badchar[256];
    badCharHeuristic(pat, m, badchar);

    size_t s = 0;
    while (s <= n - m) {
        long j = m - 1;
        while (j >= 0 && pat[j] == txt[s + j])
            j--;
        if (j < 0)
            return (char *) txt + s - 1;
        else
            s += max(1, j - badchar[(int) txt[s + j]]);
    }

    return NULL;
}

SPair *make_pair(const struct string_view *const key,
                 const struct string_view *const value)
{
    SPair *entry = malloc(sizeof(SPair));
    entry->key = string_view_dup(key);
    entry->value = string_view_dup(value);

    return entry;
}

int get_rand(void)
{
    static int counter = 0;
    static const size_t MAGIC_NUMBER = (size_t) get_rand;
    return (((size_t) &get_rand) * MAGIC_NUMBER * (++counter)) % MAGIC_NUMBER;
}

Basic_string *gen_random_dummy_string(size_t len)
{
    // Holding some non-reserved char in URL
    // To prevent it doing some unessential url decoding
    static const char table[64] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c',
        'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C',
        'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '-', '~',
    };

    Basic_string *s = malloc(sizeof(Basic_string));
    char *buf = malloc(len + 1);
    for (size_t i = 0; i < len; i++)
        buf[i] = table[get_rand() & (ARR_LEN(table) - 1)];

    s->data = buf;
    s->size = len;
    s->data[len] = 0;

    return s;
}

void html_escape_trans(Basic_string *str)
{
    if (unlikely(!str || !str->data))
        return;

    // https://www.html.am/reference/html-special-characters.cfm
    static const char escape_key[] = {
        '"', '\'', '&', '<', '>',
    };
    static const char *escape_map[] = {
        "&#34;", "&#39;", "&#38;", "&#60;", "&#62;",
    };

    int escape_counter[ARR_LEN(escape_key)] = {0};
    for (size_t i = 0; i < str->size; i++)
        for (size_t j = 0; j < ARR_LEN(escape_key); j++)
            if (unlikely(str->data[i] == escape_key[j]))
                ++escape_counter[j];

    // Add escape reserved sizes and origin str->size
    size_t new_size = str->size;
    for (size_t j = 0; j < ARR_LEN(escape_counter); j++)
        new_size += 4 * escape_counter[j];  // 4 is strlen("#34;")

    // If new_size == orig->size, it is no escape chars exist
    if (new_size == str->size)
        return;

    // Have escape chars
    char *new_buf = malloc(new_size + 1);
    char *new_buf_shadow = new_buf;
    for (size_t i = 0; i < str->size; i++) {
        for (size_t j = 0; j < ARR_LEN(escape_key); j++) {
            if (unlikely(str->data[i] == escape_key[j])) {
                memcpy(new_buf, escape_map[j], strlen(escape_map[j]));
                new_buf += 5;
                goto have_escape;
            }
        }
        *new_buf++ = str->data[i];
    have_escape:;
    }

    free(str->data);
    str->data = new_buf_shadow;
    str->size = new_size;
    str->data[new_size] = 0;
}

void newline_to_br(Basic_string *str)
{
    if (unlikely(!str || !*str->data))
        return;

    size_t counter = 0;
    for (size_t i = 0; i < str->size; i++)
        if (unlikely(str->data[i] == '\n'))
            ++counter;

    if (counter == 0)
        return;

    // Have newline char
    char *new_buf = malloc(str->size + counter * 3 + 1);
    char *new_buf_shadow = new_buf;
    for (size_t i = 0; i < str->size; i++) {
        if (unlikely(str->data[i] == '\n')) {
            strncpy(new_buf, "<br>", 4);
            new_buf += 4;
        } else {
            *new_buf++ = str->data[i];
        }
    }

    free(str->data);
    str->data = new_buf_shadow;
    str->size = str->size + counter * 3;
    str->data[str->size] = 0;
}

void sha256_string(char dst[65], const Basic_string *str)
{
    // ‘SHA256_Init’ is deprecated: Since OpenSSL 3.0
    unsigned char hash[SHA256_DIGEST_LENGTH];
    EVP_Digest(str->data, str->size, hash, NULL, EVP_sha256(), NULL);

    // hash stores the raw hash value, we should transfer to literal string for
    // comparing
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        snprintf(dst + (i * 2), 3, "%02x", hash[i]);

    dst[64] = 0;
}