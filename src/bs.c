#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bs.h"

FB_string* fbs_ctor(const char* const str) {
	size_t len = strlen(str);
	FB_string* ret = malloc(sizeof(FB_string));
	ret->data = malloc(sizeof(char) * (len + 1));
	ret->length = len;
	strncpy(ret->data, str, len + 1);
}

void fbs_resize(FB_string* str, int new_size) {
	str->data = realloc(str->data, sizeof(char) * (new_size + 1));
	str->length = new_size;
}

FB_string* fbs_combine(const FB_string* const str1, const FB_string* const str2) {
	FB_string* ret = malloc(sizeof(FB_string));
	ret->length = str1->length + str2->length;
	ret->data = malloc(sizeof(char) * (str1->length + str2->length + 1));
	strncpy(ret, str1, str1->length);
	strncpy(ret + str1->length, str2, str2->length + 1);
	return ret;
}

FB_string* fbs_get_random(uint32_t len) {
	static const char table[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c',
								  'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
								  'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C',
								  'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
								  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' };
	char random[len + 1];
	for(int i = 0; i < len; i++)
		random[i] = table[rand() % sizeof(table)];
	random[len] = '\0';
	FB_string* ret = fbs_ctor(random);
	return ret;
}

FB_string* fbs_replace(const FB_string* const origin, const FB_string* const replace_pattern,
					   const FB_string* const with) {
	FB_string* cpy_origin = fbs_ctor(origin->data);
	/* copied, pasted, and modified from stack overflow:
	 * https://stackoverflow.com/a/779960/14835280 */
	// sanity checks and initialization
	if(!origin || !replace_pattern || !replace_pattern->length)
		return NULL;
	const FB_string* const empty = fbs_ctor("");
	const FB_string* const new_with = with ? with : empty;
	char* tmp;								
	int count = 0;							// number of replacements
	char* insert_point = cpy_origin->data;	// the next insert point
	// count the number of replacements needed
	for(count = 0; tmp = strstr(insert_point, replace_pattern->data); ++count)
		insert_point = tmp + replace_pattern->length;
	char* result = tmp =
		malloc(sizeof(char)
			   * (cpy_origin->length + (new_with->length - replace_pattern->length) * count + 1));
	if(!result)
		return NULL;

	// first time through the loop, all the variable are set correctly
	// from here on,
	// tmp points to the end of the result string
	// insert_point points to the next occurrence of rep in orig
	// orig points to the remainder of orig after "end of rep"
	// len_front: distance between rep and end of last rep
	for(int len_front;; count--) {
		insert_point = strstr(cpy_origin->data, replace_pattern->data);
		len_front = insert_point - cpy_origin->data;
		tmp = strncpy(tmp, cpy_origin->data, len_front) + len_front;
		tmp = strcpy(tmp, new_with) + new_with->length;
		cpy_origin->data += len_front + replace_pattern->length;  // move to next "end of rep"
	}
	strcpy(tmp, cpy_origin->data);
	fbs_dtor(cpy_origin);
	fbs_dtor(empty);
	const FB_string* const ret = fbs_ctor(result);
	free(result);
	return ret;
}

void fbs_dtor(FB_string* str) {
	if(str->data)
		free(str->data);
	free(str);
}

int fbs_is_same(const FB_string* const str1, const FB_string* const str2) {
	char *elem1 = str1->data, *elem2 = str2->data;
	for(; elem1 && elem2; elem1++, elem2++) {
		if(*elem1 != *elem2)
			return false;
	}
	return elem1 == elem2;
}

char* bsNew(const char* str) {
	size_t len = strlen(str);
	char* bs = malloc(sizeof(char) * (BS_HEADER_LEN + len + 1));
	assert(bs);
	bs += BS_HEADER_LEN;

	strncpy(bs, str, len);
	bsSetLen(bs, len);

	return bs;
}

char* bsNewLen(char* buf, size_t len) {
	char* bs = malloc(sizeof(char) * (BS_HEADER_LEN + len + 1));
	assert(bs);
	bs += BS_HEADER_LEN;

	strncpy(bs, buf, len);
	bsSetLen(bs, len);

	return bs;
}

char* bsCat(char* bs1, char* bs2) {
	size_t len1 = bsGetLen(bs1);
	size_t len2 = bsGetLen(bs2);
	size_t len = len1 + len2;

	char* bs = malloc(sizeof(char) * (BS_HEADER_LEN + len + 1));
	assert(bs);
	bs += BS_HEADER_LEN;

	strncpy(bs, bs1, len1);
	strncpy(bs + len1, bs2, len2);

	bsSetLen(bs, len);

	return bs;
}

char* bsRandom(uint32_t len, char* suffix) {
	static const char table[62] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c',
									'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
									'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C',
									'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
									'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' };

	char* bs = malloc(sizeof(char) * (BS_HEADER_LEN + len + 1));
	assert(bs);
	bs += BS_HEADER_LEN;

	bsSetLen(bs, len);

	for(uint32_t i = 0; i < len; i++)
		bs[i] = table[rand() % sizeof(table)];

	if(suffix)
		bsLCat(&bs, suffix);

	return bs;
}

char* bsEscape(const char* bs) {
	char* copy = bsNew(bs);
	char* res = bsNew("");

	char* c = copy;
	char* p = copy;

	while(*c != '\0') {
		if(*c == '<') {
			*c = '\0';
			bsLCat(&res, p);
			bsLCat(&res, "&lt;");
			p = c + 1;
		}
		else if(*c == '>') {
			*c = '\0';
			bsLCat(&res, p);
			bsLCat(&res, "&gt;");
			p = c + 1;
		}
		c++;
	}

	bsLCat(&res, p);
	bsDel(copy);
	return res;
}

void bsLCat(char** orig, char* s) {
	const size_t lenO = bsGetLen(*orig);
	size_t lenS = strlen(s);
	size_t len = lenO + lenS;

	*orig = (char*)realloc(*orig - BS_HEADER_LEN, sizeof(char) * (BS_HEADER_LEN + len + 1));
	assert(*orig);
	*orig += BS_HEADER_LEN;

	strncpy(*orig + lenO, s, lenS);
	bsSetLen(*orig, len);
}

void bsDel(char* bs) {
	free(bs - BS_HEADER_LEN);
}

void bsSetLen(char* bs, uint32_t len) {
	*(bs + 0 - BS_HEADER_LEN) = len >> 24 & 0xFF;
	*(bs + 1 - BS_HEADER_LEN) = len >> 16 & 0xFF;
	*(bs + 2 - BS_HEADER_LEN) = len >> 8 & 0xFF;
	*(bs + 3 - BS_HEADER_LEN) = len & 0xFF;
	*(bs + len) = '\0';
}

char* bsNewline2BR(const char* bs) {
	char* copy = bsNew(bs);
	char* res = bsNew("");

	char* c = copy;
	char* p = copy;

	while(*c != '\0') {
		if(*c == '\n') {
			*c = '\0';
			bsLCat(&res, p);
			bsLCat(&res, "<br>");
			p = c + 1;
		}
		++c;
	}

	bsLCat(&res, p);
	bsDel(copy);
	return res;
}

uint32_t bsGetLen(const char* bs) {
	return bs ? (((char)*(bs + 0 - BS_HEADER_LEN) & 0xFF) << 24)
					| (((char)*(bs + 1 - BS_HEADER_LEN) & 0xFF) << 16)
					| (((char)*(bs + 2 - BS_HEADER_LEN) & 0xFF) << 8)
					| (((char)*(bs + 3 - BS_HEADER_LEN) & 0xFF))
			  : 0;
}
