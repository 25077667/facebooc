#include "utility.h"
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

// Reference: http://www.cse.yorku.ca/~oz/hash.html
int string_hash(const char* str) {
	int hash = 5381;
	int c;

	while((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

// sdbm
int obj_hash(const void* data, size_t size) {
	unsigned long hash = 0;

	for(size_t i = 0; i < size; i++) {
		hash = ((int*)data)[i] + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

void mem_canary_alert(const char* msg) {
	puts(msg);
#ifdef __SANITIZE_ADDRESS__
	const char* canary = NULL;
	*canary;
#else
	void* buffer[64];
	char** symbols;

	int num = backtrace(buffer, 64);
	printf("backtrace() returned %d addresses\n", num);

	symbols = backtrace_symbols(buffer, num);
	if(symbols == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	for(int j = 0; j < num; j++)
		printf("%s\n", symbols[j]);

	free(symbols);
#endif
}