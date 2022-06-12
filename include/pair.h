#ifndef __PAIR_H__
#define __PAIR_H__
#include <stdint.h>

typedef struct Pair Pair;
typedef struct PPair PPair;
typedef struct SPair SPair;
typedef struct SSPair SSPair;

struct Pair {
	uint64_t key;
	void* value;
};

struct PPair {
	void* key;
	void* value;
};

// String pair, string as the key
// The key is immutable
struct SPair {
	char* key;
	void* value;
};

struct SSPair {
	char* key;
	char* value;
};

void Pair_delete(Pair* p);
void PPair_delete(PPair* p);
void SPair_delete(SPair* p);
void SSPair_delete(SSPair* p);

#define eXpire_pair(key_, value_, type) \
	&(type) {                           \
		.key = key_, .value = value_    \
	}

#endif