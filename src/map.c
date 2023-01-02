#include "map.h"
#include <stdlib.h>
#include <string.h>
#include "utility.h"

// Map is a wrapper to a rbtree
struct Map {
    rbtree_t *tree;  // Will put pair into it
};

static inline int cmpf(const void *a, const void *b)
{
    const Pair *pa = (const Pair *) a;
    const Pair *pb = (const Pair *) b;
    return pb->key - pa->key;
}

Map *Map_new()
{
    Map *m = malloc(sizeof(Map));
    m->tree = rbtree_create(cmpf);
    return m;
}

void Map_delete(Map *m)
{
    Pair *p = NULL;
    RBTREE_FOR(p, Pair, m->tree)
    {
        Pair_delete(p);
    }
    free(m->tree);
    free(m);
    m = NULL;
}

void *Map_get(Map *m, uint64_t key)
{
    if (unlikely(!m))
        return NULL;

    Pair tmp = {.key = key, .value = NULL};
    const rbnode_t *result = rbtree_search(m->tree, &tmp);
    return ((const Pair *) result->key)->value;
}

_Bool Map_insert(Map *m, const Pair *p)
{
    if (unlikely(!m))
        return NULL;

    rbnode_t *new_node = malloc(sizeof(rbnode_t));
    memcpy(new_node, &rbtree_null_node, sizeof(rbnode_t));

    void *new_key = memdup(p, sizeof(Pair));
    new_node->key = new_key;

    void *res = rbtree_insert(m->tree, new_node);
    if (unlikely(res == NULL)) {
        free(new_key);
        free(new_node);
    }

    return true;
}

_Bool Map_insert_move(Map *m, Pair *p)
{
    if (unlikely(!m))
        return NULL;

    rbnode_t *new_node = malloc(sizeof(rbnode_t));
    memcpy(new_node, &rbtree_null_node, sizeof(rbnode_t));
    new_node->key = p;

    void *res = rbtree_insert(m->tree, new_node);
    if (unlikely(res == NULL))
        free(new_node);

    return true;
}

_Bool Map_erase(Map *m, uint64_t key)
{
    if (unlikely(!m))
        return NULL;

    Pair tmp = {.key = key, .value = NULL};
    rbnode_t *node = rbtree_delete(m->tree, &tmp);
    if (likely(node)) {
        Pair_delete((Pair *) node->key);
        free(node);
        node = (rbnode_t *) 1;
    }
    return !!node;
}

size_t Map_size(const Map *m)
{
    return m->tree->count;
}

Map *Map_P_new()
{
    return Map_new();
}

void Map_P_delete(Map *m)
{
    PPair *p = NULL;
    RBTREE_FOR(p, PPair, m->tree)
    {
        PPair_delete(p);
    }
    free(m->tree);
    free(m);
    m = NULL;
}

void *Map_P_get(Map *m, const void *key)
{
    return Map_get(m, (uint64_t) key);
}

_Bool Map_P_insert(Map *m, const PPair *p)
{
    if (unlikely(!m))
        return NULL;

    rbnode_t *new_node = malloc(sizeof(rbnode_t));
    memcpy(new_node, &rbtree_null_node, sizeof(rbnode_t));

    void *new_key = memdup(p, sizeof(PPair));
    new_node->key = new_key;

    void *res = rbtree_insert(m->tree, new_node);
    if (unlikely(res == NULL)) {
        free(new_key);
        free(new_node);
    }

    return true;
}

_Bool Map_P_insert_move(Map *m, PPair *p)
{
    return Map_insert_move(m, (Pair *) p);
}

_Bool Map_P_erase(Map *m, void *key)
{
    if (unlikely(!m))
        return NULL;

    PPair tmp = {.key = key, .value = NULL};
    rbnode_t *node = rbtree_delete(m->tree, &tmp);
    if (likely(node)) {
        PPair_delete((PPair *) node->key);
        free(node);
        node = (rbnode_t *) 1;
    }
    return !!node;
}

size_t Map_P_size(const Map *m)
{
    return Map_size(m);
}