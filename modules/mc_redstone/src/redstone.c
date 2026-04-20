/*
 * redstone.c -- Core redstone system: power map, DI callbacks, update logic.
 *
 * Power levels are stored as 4 bits (0-15) per block position in a hash map.
 * Block query/set callbacks are injected at init time so this module never
 * imports mc_world directly.
 */

#include "mc_redstone.h"
#include "mc_block.h"
#include <string.h>

/* ---- internal header shared with propagate.asm / components.c ---- */
#include "redstone_internal.h"

/* ---- Static state ---- */

static mc_redstone_block_query_fn g_query_fn;
static mc_redstone_block_set_fn   g_set_fn;

/*
 * Power map: open-addressing hash table keyed by block_pos_t.
 * Each entry stores the position and a 4-bit power level.
 */
#define POWER_MAP_CAPACITY 4096

typedef struct {
    block_pos_t pos;
    uint8_t     power;   /* 0-15 */
    uint8_t     occupied;
} power_entry_t;

static power_entry_t g_power_map[POWER_MAP_CAPACITY];

/* ---- Hash helpers ---- */

static power_entry_t *power_map_find(block_pos_t pos)
{
    uint32_t idx = rs_pos_hash(pos) % POWER_MAP_CAPACITY;
    for (uint32_t i = 0; i < POWER_MAP_CAPACITY; i++) {
        uint32_t slot = (idx + i) % POWER_MAP_CAPACITY;
        if (!g_power_map[slot].occupied) {
            return NULL;
        }
        if (rs_pos_equal(g_power_map[slot].pos, pos)) {
            return &g_power_map[slot];
        }
    }
    return NULL;
}

static power_entry_t *power_map_insert(block_pos_t pos)
{
    uint32_t idx = rs_pos_hash(pos) % POWER_MAP_CAPACITY;
    for (uint32_t i = 0; i < POWER_MAP_CAPACITY; i++) {
        uint32_t slot = (idx + i) % POWER_MAP_CAPACITY;
        if (!g_power_map[slot].occupied) {
            g_power_map[slot].pos      = pos;
            g_power_map[slot].power    = 0;
            g_power_map[slot].occupied = 1;
            return &g_power_map[slot];
        }
        if (rs_pos_equal(g_power_map[slot].pos, pos)) {
            return &g_power_map[slot];
        }
    }
    return NULL; /* table full */
}

/* ---- Accessor used by propagate (C-callable) ---- */

mc_redstone_block_query_fn mc_redstone_get_query_fn(void)
{
    return g_query_fn;
}

void mc_redstone_power_map_set(block_pos_t pos, uint8_t level)
{
    power_entry_t *e = power_map_insert(pos);
    if (e) {
        e->power = level & 0x0F;
    }
}

uint8_t mc_redstone_power_map_get(block_pos_t pos)
{
    const power_entry_t *e = power_map_find(pos);
    return e ? e->power : 0;
}

/* ---- Public API ---- */

mc_error_t mc_redstone_init(mc_redstone_block_query_fn query,
                            mc_redstone_block_set_fn   set)
{
    if (!query || !set) {
        return MC_ERR_INVALID_ARG;
    }
    g_query_fn = query;
    g_set_fn   = set;
    memset(g_power_map, 0, sizeof(g_power_map));
    return MC_OK;
}

void mc_redstone_shutdown(void)
{
    g_query_fn = NULL;
    g_set_fn   = NULL;
    memset(g_power_map, 0, sizeof(g_power_map));
}

uint8_t mc_redstone_get_power(block_pos_t pos)
{
    return mc_redstone_power_map_get(pos);
}

/*
 * mc_redstone_update -- declared in the public header.
 * Implemented in propagate.asm (mc_redstone_propagate_bfs) which we call here
 * as a thin wrapper.
 */
void mc_redstone_update(block_pos_t pos)
{
    if (!g_query_fn) {
        return;
    }
    mc_redstone_propagate_bfs(pos);
}
