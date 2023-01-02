#include "base64.h"

#include <stdint.h>
#include <stdlib.h>

static const uint16_t encoding_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
};

// -1 is invalid byte
static const uint16_t decoding_table[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1,
    -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
};

Basic_string *base64_encode(const void *data, size_t len)
{
    size_t output_len = (len + 2) / 3 * 4;
    Basic_string *out = malloc(sizeof(Basic_string));
    out->data = malloc(output_len + 1);
    out->size = output_len;

    char *out_data = out->data;
    for (size_t i = 0; len - i > 2; i += 3) {
        // Process triple byte in once
        uint32_t triple = ((*((uint8_t *) data + i)) << 0x10) |
                          ((*((uint8_t *) data + i + 1)) << 0x08) |
                          (*((uint8_t *) data + i + 2));

        // 3 byte of origin data are mapped on to 4 encoding elements
        *out_data++ = encoding_table[(triple >> 3 * 6) & 0x3F];
        *out_data++ = encoding_table[(triple >> 2 * 6) & 0x3F];
        *out_data++ = encoding_table[(triple >> 1 * 6) & 0x3F];
        *out_data++ = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    // The number of padding '=':
    // If len % 3 == 0   ->  0 padding
    // If len % 3 == 1   ->  2 padding
    // If len % 3 == 2   ->  1 padding
    if (len % 3 == 1) {
        *out_data++ = encoding_table[(*((uint8_t *) data + len - 1)) >> 2];
        *out_data++ =
            encoding_table[((*((uint8_t *) data + len - 1)) & 0b11) << 4];
        *out_data++ = '=';
        *out_data++ = '=';
    } else if (len % 3 == 2) {
        *out_data++ = encoding_table[(*((uint8_t *) data + len - 2)) >> 2];
        *out_data++ =
            encoding_table[(((*((uint8_t *) data + len - 2)) & 0b11) << 4) |
                           (((*((uint8_t *) data + len - 1)) & 0xF0) >> 4)];
        *out_data++ =
            encoding_table[((*((uint8_t *) data + len - 1)) & 0x0F) << 2];
        *out_data++ = '=';
    }

    out->data[output_len] = 0;
    return out;
}

void base64_decode(void **dst, size_t *dst_len, const Basic_string *src)
{
    *dst_len = src->size / 4 * 3;
    size_t stripped_padding_len = src->size;

    // Ignore it if there has last 2 padding(s)
    if (src->data[src->size - 1] == '=') {
        --*dst_len;
        --stripped_padding_len;
    }
    if (src->data[src->size - 2] == '=') {
        --*dst_len;
        --stripped_padding_len;
    }

    *dst = malloc(*dst_len + 1);
    char *dst_shadow = *dst;
    for (size_t i = 0; stripped_padding_len - i > 3; i += 4) {
        // Process four encoding elements to 3 byte in once
        uint32_t triple =
            (decoding_table[(uint8_t) src->data[i]] << 3 * 6) |
            (decoding_table[(uint8_t) src->data[i + 1]] << 2 * 6) |
            (decoding_table[(uint8_t) src->data[i + 2]] << 1 * 6) |
            (decoding_table[(uint8_t) src->data[i + 3]] << 0 * 6);

        *dst_shadow++ = (triple >> 2 * 8) & 0xFF;
        *dst_shadow++ = (triple >> 1 * 8) & 0xFF;
        *dst_shadow++ = (triple >> 0 * 8) & 0xFF;
    }

    int padding_len = src->size - stripped_padding_len;
    if (padding_len == 1) {
        // Have 2 char
        *dst_shadow++ =
            (decoding_table[(uint8_t) src->data[stripped_padding_len - 3]]
             << 2) |
            ((decoding_table[(uint8_t) src->data[stripped_padding_len - 2]] >>
              4) &
             0b11);
        *dst_shadow++ =
            ((decoding_table[(uint8_t) src->data[stripped_padding_len - 2]]
              << 4) &
             0xF0) |
            (decoding_table[(uint8_t) src->data[stripped_padding_len - 1]] >>
             2);
    } else if (padding_len == 2) {
        // Have 1 char
        *dst_shadow++ =
            (decoding_table[(uint8_t) src->data[stripped_padding_len - 2]]
             << 2) |
            ((decoding_table[(uint8_t) src->data[stripped_padding_len - 1]] >>
              4) &
             0b11);
    }

    *dst_shadow = 0;
}

Basic_string *base64_decode2str(const Basic_string *src)
{
    Basic_string *ret = malloc(sizeof(Basic_string));
    base64_decode((void **) &ret->data, &ret->size, src);
    return ret;
}