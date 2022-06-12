#include "pair.h"
#include <stdlib.h>

void Pair_delete(Pair* p) {
	free(p->value);
	free(p);
}

void PPair_delete(PPair* p) {
	return SPair_delete((SPair*)p);
}

void SPair_delete(SPair* p) {
	if(!p)
		return;
	free(p->key);
	free(p->value);
	free(p);
	p = NULL;
}

void SSPair_delete(SSPair* p) {
	if(!p)
		return;
	free(p->key);
	free(p->value);
	free(p);
	p = NULL;
}