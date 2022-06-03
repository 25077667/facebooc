#ifndef BS_H
#define BS_H

#include <stdint.h>
#include <stdlib.h>

#include "list.h"

typedef struct FB_string FB_string;

struct FB_string {
	size_t length;
	char* data;
};

FB_string* fbs_ctor(const char* const str);
void fbs_resize(FB_string* str, int new_size);
FB_string* fbs_combine(const FB_string* const str1, const FB_string* const str2);
FB_string* fbs_get_random(uint32_t len);
FB_string* fbs_replace(const FB_string* const origin, const FB_string* const replace_pattern,
					   const FB_string* const with);
int fbs_is_same(const FB_string* const str1, const FB_string* const str2);

#define BS_HEADER_LEN 4

char* bsNew(const char*);
char* bsNewLen(char*, size_t);
char* bsCat(char*, char*);
char* bsRandom(uint32_t, char*);
char* bsEscape(const char*);
char* bsNewline2BR(const char*);
void bsLCat(char**, char*);
void bsDel(char*);
void bsSetLen(char*, uint32_t);
uint32_t bsGetLen(const char*);

#endif
