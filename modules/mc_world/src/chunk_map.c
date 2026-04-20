#include "mc_world.h"
#include "world_internal.h"
#include <stdlib.h>
#include <string.h>

/* Global world state */
world_state_t g_world = {0};

uint32_t chunk_pos_hash(chunk_pos_t pos)
{
    return (uint32_t)((uint32_t)pos.x * 73856093u) ^ ((uint32_t)pos.z * 19349663u);
}

bool chunk_pos_equal(chunk_pos_t a, chunk_pos_t b)
{
    return a.x == b.x && a.z == b.z;
}

static mc_error_t chunk_map_init(uint32_t capacity)
{
    g_world.entries = calloc(capacity, sizeof(chunk_map_entry_t));
    if (!g_world.entries) {
        return MC_ERR_OUT_OF_MEMORY;
    }
    g_world.capacity = capacity;
    g_world.count = 0;
    return MC_OK;
}

static void chunk_map_destroy(void)
{
    if (!g_world.entries) {
        return;
    }
    for (uint32_t i = 0; i < g_world.capacity; i++) {
        if (g_world.entries[i].occupied && g_world.entries[i].chunk) {
            free(g_world.entries[i].chunk);
        }
    }
    free(g_world.entries);
    g_world.entries = NULL;
    g_world.capacity = 0;
    g_world.count = 0;
}

static mc_error_t chunk_map_grow(void)
{
    uint32_t new_capacity = g_world.capacity * 2;
    chunk_map_entry_t *new_entries = calloc(new_capacity, sizeof(chunk_map_entry_t));
    if (!new_entries) {
        return MC_ERR_OUT_OF_MEMORY;
    }

    for (uint32_t i = 0; i < g_world.capacity; i++) {
        if (!g_world.entries[i].occupied) {
            continue;
        }
        uint32_t hash = chunk_pos_hash(g_world.entries[i].key);
        uint32_t idx = hash & (new_capacity - 1);
        while (new_entries[idx].occupied) {
            idx = (idx + 1) & (new_capacity - 1);
        }
        new_entries[idx] = g_world.entries[i];
    }

    free(g_world.entries);
    g_world.entries = new_entries;
    g_world.capacity = new_capacity;
    return MC_OK;
}

chunk_t *world_find_chunk(chunk_pos_t pos)
{
    if (!g_world.entries || g_world.capacity == 0) {
        return NULL;
    }
    uint32_t hash = chunk_pos_hash(pos);
    uint32_t idx = hash & (g_world.capacity - 1);
    uint32_t start = idx;

    do {
        if (!g_world.entries[idx].occupied) {
            return NULL;
        }
        if (chunk_pos_equal(g_world.entries[idx].key, pos)) {
            return g_world.entries[idx].chunk;
        }
        idx = (idx + 1) & (g_world.capacity - 1);
    } while (idx != start);

    return NULL;
}

mc_error_t mc_world_init(uint32_t seed)
{
    if (g_world.initialized) {
        mc_world_shutdown();
    }
    mc_error_t err = chunk_map_init(CHUNK_MAP_INITIAL_CAPACITY);
    if (err != MC_OK) {
        return err;
    }
    g_world.seed = seed;
    g_world.initialized = true;
    return MC_OK;
}

void mc_world_shutdown(void)
{
    chunk_map_destroy();
    g_world.seed = 0;
    g_world.initialized = false;
}

const chunk_t *mc_world_get_chunk(chunk_pos_t pos)
{
    return world_find_chunk(pos);
}

mc_error_t mc_world_load_chunk(chunk_pos_t pos)
{
    if (!g_world.initialized) {
        return MC_ERR_NOT_READY;
    }

    /* Check if already loaded */
    if (world_find_chunk(pos) != NULL) {
        return MC_ERR_ALREADY_EXISTS;
    }

    /* Grow if load factor exceeds 70% */
    if (g_world.count * 10 >= g_world.capacity * 7) {
        mc_error_t err = chunk_map_grow();
        if (err != MC_OK) {
            return err;
        }
    }

    /* Allocate and initialize the chunk */
    chunk_t *chunk = calloc(1, sizeof(chunk_t));
    if (!chunk) {
        return MC_ERR_OUT_OF_MEMORY;
    }
    chunk->pos = pos;
    chunk->state = CHUNK_STATE_READY;

    /* Insert into hash map */
    uint32_t hash = chunk_pos_hash(pos);
    uint32_t idx = hash & (g_world.capacity - 1);
    while (g_world.entries[idx].occupied) {
        idx = (idx + 1) & (g_world.capacity - 1);
    }
    g_world.entries[idx].key = pos;
    g_world.entries[idx].chunk = chunk;
    g_world.entries[idx].occupied = true;
    g_world.count++;

    return MC_OK;
}

void mc_world_unload_chunk(chunk_pos_t pos)
{
    if (!g_world.entries || g_world.capacity == 0) {
        return;
    }

    uint32_t hash = chunk_pos_hash(pos);
    uint32_t idx = hash & (g_world.capacity - 1);
    uint32_t start = idx;

    do {
        if (!g_world.entries[idx].occupied) {
            return; /* Not found */
        }
        if (chunk_pos_equal(g_world.entries[idx].key, pos)) {
            /* Free the chunk and mark entry empty */
            free(g_world.entries[idx].chunk);
            g_world.entries[idx].occupied = false;
            g_world.entries[idx].chunk = NULL;
            g_world.count--;

            /* Re-insert displaced entries (Robin Hood deletion) */
            uint32_t next = (idx + 1) & (g_world.capacity - 1);
            while (g_world.entries[next].occupied) {
                chunk_map_entry_t displaced = g_world.entries[next];
                g_world.entries[next].occupied = false;
                g_world.entries[next].chunk = NULL;
                g_world.count--;

                /* Re-insert */
                uint32_t h = chunk_pos_hash(displaced.key);
                uint32_t ri = h & (g_world.capacity - 1);
                while (g_world.entries[ri].occupied) {
                    ri = (ri + 1) & (g_world.capacity - 1);
                }
                g_world.entries[ri] = displaced;
                g_world.count++;

                next = (next + 1) & (g_world.capacity - 1);
            }
            return;
        }
        idx = (idx + 1) & (g_world.capacity - 1);
    } while (idx != start);
}
