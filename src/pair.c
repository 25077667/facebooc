#include "pair.h"
#include <stdlib.h>

void Pair_delete(Pair *p)
{
    if (!p)
        return;
    free(p->value);
    free(p);
}

void PPair_delete(PPair *p)
{
    if (!p)
        return;
    free(p->key);
    free(p->value);
    free(p);
}

void SPair_delete(SPair *p)
{
    if (!p)
        return;
    free(p->key);
    free(p->value);
    free(p);
}

void SSPair_delete(SSPair *p)
{
    if (!p)
        return;
    free(p->key);
    free(p->value);
    free(p);
}
