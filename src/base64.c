#include "base64.h"
#include <stdint.h>
#include <stdlib.h>

/*
 * Base64 encoding table: maps values 0-63 to the corresponding Base64
 * character.
 */
static const char encoding_table[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"
static const int decoding_table[256] = {
    [0 ... 255] = -1,  // Initialize all entries to -1.
    ['A'] = 0,        ['B'] = 1,  ['C'] = 2,  ['D'] = 3,  ['E'] = 4,
    ['F'] = 5,        ['G'] = 6,  ['H'] = 7,  ['I'] = 8,  ['J'] = 9,
    ['K'] = 10,       ['L'] = 11, ['M'] = 12, ['N'] = 13, ['O'] = 14,
    ['P'] = 15,       ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19,
    ['U'] = 20,       ['V'] = 21, ['W'] = 22, ['X'] = 23, ['Y'] = 24,
    ['Z'] = 25,       ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29,
    ['e'] = 30,       ['f'] = 31, ['g'] = 32, ['h'] = 33, ['i'] = 34,
    ['j'] = 35,       ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39,
    ['o'] = 40,       ['p'] = 41, ['q'] = 42, ['r'] = 43, ['s'] = 44,
    ['t'] = 45,       ['u'] = 46, ['v'] = 47, ['w'] = 48, ['x'] = 49,
    ['y'] = 50,       ['z'] = 51, ['0'] = 52, ['1'] = 53, ['2'] = 54,
    ['3'] = 55,       ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59,
    ['8'] = 60,       ['9'] = 61, ['+'] = 62, ['/'] = 63,
};
#pragma GCC diagnostic pop

Basic_string *base64_encode(const void *data, size_t len)
{
    const unsigned char *input = (const unsigned char *) data;
    size_t output_len = ((len + 2) / 3) * 4;
    Basic_string *out = malloc(sizeof(Basic_string));
    if (!out)
        return NULL;

    out->data = malloc(output_len + 1);
    if (!out->data) {
        free(out);
        return NULL;
    }
    out->size = output_len;

    char *out_data = out->data;
    size_t i;
    /* Process complete 3-byte blocks. */
    for (i = 0; i + 2 < len; i += 3) {
        uint32_t triple = (input[i] << 16) | (input[i + 1] << 8) | input[i + 2];
        *out_data++ = encoding_table[(triple >> 18) & 0x3F];
        *out_data++ = encoding_table[(triple >> 12) & 0x3F];
        *out_data++ = encoding_table[(triple >> 6) & 0x3F];
        *out_data++ = encoding_table[triple & 0x3F];
    }

    /* Process the remaining bytes and add padding as necessary. */
    if (len % 3 == 1) {
        uint8_t a = input[len - 1];
        *out_data++ = encoding_table[a >> 2];
        *out_data++ = encoding_table[(a & 0x03) << 4];
        *out_data++ = '=';
        *out_data++ = '=';
    } else if (len % 3 == 2) {
        uint8_t a = input[len - 2];
        uint8_t b = input[len - 1];
        *out_data++ = encoding_table[a >> 2];
        *out_data++ = encoding_table[((a & 0x03) << 4) | (b >> 4)];
        *out_data++ = encoding_table[(b & 0x0F) << 2];
        *out_data++ = '=';
    }

    out->data[output_len] = '\0';
    return out;
}

void base64_decode(void **dst, size_t *dst_len, const Basic_string *src)
{
    const char *input = src->data;
    size_t input_len = src->size;

    /* Calculate the number of padding characters. */
    int pad = 0;
    if (input_len && input[input_len - 1] == '=')
        pad++;
    if (input_len > 1 && input[input_len - 2] == '=')
        pad++;

    /* Each 4 characters encode 3 bytes, but padding reduces the count. */
    *dst_len = (input_len / 4) * 3 - pad;
    *dst = malloc(*dst_len + 1);
    if (!*dst)
        return;
    unsigned char *out = (unsigned char *) *dst;

    size_t i = 0, j = 0;
    /* Process all complete blocks except the possibly incomplete final block.
     */
    for (; i < input_len; i += 4) {
        int d0 = decoding_table[(unsigned char) input[i]];
        int d1 = decoding_table[(unsigned char) input[i + 1]];
        int d2 = (i + 2 < input_len && input[i + 2] != '=')
                     ? decoding_table[(unsigned char) input[i + 2]]
                     : 0;
        int d3 = (i + 3 < input_len && input[i + 3] != '=')
                     ? decoding_table[(unsigned char) input[i + 3]]
                     : 0;

        uint32_t triple = (d0 << 18) | (d1 << 12) | (d2 << 6) | d3;

        if (j < *dst_len)
            out[j++] = (triple >> 16) & 0xFF;
        if (j < *dst_len)
            out[j++] = (triple >> 8) & 0xFF;
        if (j < *dst_len)
            out[j++] = triple & 0xFF;
    }
    out[*dst_len] = '\0';
}

Basic_string *base64_decode2str(const Basic_string *src)
{
    Basic_string *ret = malloc(sizeof(Basic_string));
    if (!ret)
        return NULL;
    base64_decode((void **) &ret->data, &ret->size, src);
    return ret;
}
