
#include "hash_map.h"
#include "utility.h"
#include <stdint.h>
#include <string.h>

// Refers to: https://github.com/benhoyt/ht/
// Hash map structure: create with Hash_map_new, free with Hash_map_delete.
struct Hash_map {
	SPair** entries;  // hash slots
	size_t cap;		  // size of _entries array
	size_t len;		  // number of items in hash table
};

static const int INITIAL_CAPACITY = 64;	 // must not be zero, and power of 2

Hash_map* Hash_map_new() {
	Hash_map* m = malloc(sizeof(Hash_map));

	m->len = 0;
	m->cap = INITIAL_CAPACITY;
	m->entries = calloc(m->cap, sizeof(SPair*));
	return m;
}

void Hash_map_delete(Hash_map* map) {
	// First free allocated keys.
	for(size_t i = 0; i < map->cap; i++)
		if(map->entries[i])
			SPair_delete(map->entries[i]);

	// Then free entries array and map itself.
	free(map->entries);
	free(map);
}

static const unsigned long FNV_OFFSET = 14695981039346656037UL;
static const unsigned long FNV_PRIME = 1099511628211UL;

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
static inline uint64_t hash_key(const char* key) {
	uint64_t hash = FNV_OFFSET;
	for(const char* p = key; *p; p++) {
		hash ^= (uint64_t)(unsigned char)(*p);
		hash *= FNV_PRIME;
	}
	return hash;
}

static inline size_t get_index(size_t cap, const char* key) {
	// AND hash with capacity-1 to ensure it's within entries array.
	uint64_t hash = hash_key(key);
	return (size_t)(hash & (uint64_t)(cap - 1));
}

void* Hash_map_get(const Hash_map* map, const char* key) {
	size_t index = get_index(map->cap, key);

	while(map->entries[index]) {
		if(strcmp(key, map->entries[index]->key) == 0)
			return map->entries[index]->value;	// Found key, return value.

		// Key wasn't in this slot, move to next (linear probing).
		++index;
		if(unlikely(index >= map->cap))	 // Use branching to enhance performance
			index = 0;					 // At end of entries array, wrap around.
	}

	return NULL;
}

// Move existed SPair to next available space
// p is in the entry array (same pointer)
static inline _Bool hash_map_move_entry(SPair** new_entries, size_t cap, SPair* p) {
	size_t index = get_index(cap, p->key);

	while(new_entries[index]) {
		++index;
		if(unlikely(index >= cap))	// Use branching to enhance performance
			index = 0;				// At end of entries array, wrap around.
		if(new_entries[index] == p)
			return false;
	}

	new_entries[index] = p;
	p = NULL;
	return true;
}

// Internal function to set an entry (without expanding table).
// @p must not be in the entries
// Return false if p is in the entry
static inline _Bool hash_map_set_entry(SPair** entries, size_t cap, const SPair* p) {
	size_t index = get_index(cap, p->key);

	// Loop until we find an empty entry.
	while(entries[index]) {
		if(unlikely(p == entries[index]))
			return false;
		else if(strcmp(p->key, entries[index]->key) == 0) {
			SPair_delete(entries[index]);  // Found key (it already exists), overwrite pair pointer.
			break;
		}
		// Key wouldn't in this slot, move to next (linear probing).
		++index;
		if(unlikely(index >= cap))	// Use branching to enhance performance
			index = 0;				// At end of entries array, wrap around.
	}

	// Copy p into hash map
	entries[index] = malloc(sizeof(SPair));
	((SPair*)(entries[index]))->key = p->key;
	((SPair*)(entries[index]))->value = p->value;
	return true;
}

// Expand hash table to twice its current size. Return true on success,
// false if out of memory.
static bool hash_map_expand(Hash_map* map) {
	// Allocate new entries array.
	size_t new_cap = map->cap << 1;
	if(new_cap < map->cap)
		return false;  // overflow (capacity would be too big)

	SPair** new_entries = calloc(new_cap, sizeof(SPair*));

	// Iterate entries, move all non-empty ones to new map's entries.
	for(size_t i = 0; i < map->cap; i++) {
		SPair* entry = map->entries[i];
		if(entry)
			hash_map_move_entry(new_entries, new_cap, entry);
	}

	// Free old entries array and update this map's details.
	free(map->entries);
	map->entries = new_entries;
	map->cap = new_cap;
	return true;
}

_Bool Hash_map_insert(Hash_map* map, const SPair* p) {
	// If length will exceed half of current capacity, expand it.
	if(map->len >= map->cap >> 1)
		if(!hash_map_expand(map))
			return NULL;

	// Set entry and update length.
	_Bool res = hash_map_set_entry(map->entries, map->cap, p);
	map->len += res;
	return res;
}

_Bool Hash_map_insert_move(Hash_map* map, SPair* p) {
	// If length will exceed half of current capacity, expand it.
	if(map->len >= map->cap >> 1)
		if(!hash_map_expand(map))
			return NULL;

	// Set entry and update length.
	_Bool res = hash_map_move_entry(map->entries, map->cap, p);
	map->len += res;
	return res;
}

static inline void move_last_same_hash(SPair** hole, size_t hash) {
	size_t index = 1;

	while(hole[index] && hash_key(hole[index]->key) == hash) {
		++index;
	}

	// Move last same hash to hole
	*hole = hole[index - 1];
	hole[index - 1] = NULL;
}

_Bool Hash_map_erase(Hash_map* map, const char* key) {
	size_t index = get_index(map->cap, key);

	while(map->entries[index]) {
		if(strcmp(key, map->entries[index]->key) == 0) {
			SPair_delete(map->entries[index]);
			map->entries[index] = NULL;

			move_last_same_hash(&map->entries[index], hash_key(key));
		}
	}

	return true;
}

int Hash_map_size(const Hash_map* map) {
	return map->len;
}
