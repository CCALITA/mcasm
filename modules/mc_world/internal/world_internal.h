#ifndef WORLD_INTERNAL_H
#define WORLD_INTERNAL_H

#include "mc_types.h"
#include "mc_error.h"
#include <stdbool.h>

#define CHUNK_MAP_INITIAL_CAPACITY 256

typedef struct {
    chunk_pos_t key;
    chunk_t    *chunk;
    bool        occupied;
} chunk_map_entry_t;

typedef struct {
    uint32_t           seed;
    chunk_map_entry_t *entries;
    uint32_t           capacity;
    uint32_t           count;
    bool               initialized;
} world_state_t;

/* Single global world state */
extern world_state_t g_world;

/* Internal helpers */
uint32_t chunk_pos_hash(chunk_pos_t pos);
bool     chunk_pos_equal(chunk_pos_t a, chunk_pos_t b);

/* Chunk map operations (used by chunk_map.c, exposed for block_access.c etc.) */
chunk_t *world_find_chunk(chunk_pos_t pos);

#endif /* WORLD_INTERNAL_H */
