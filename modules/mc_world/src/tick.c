#include "mc_world.h"
#include "world_internal.h"

uint32_t mc_world_loaded_chunk_count(void)
{
    return g_world.count;
}

void mc_world_get_dirty_chunks(chunk_pos_t *out, uint32_t max, uint32_t *count)
{
    uint32_t found = 0;

    if (!g_world.entries || !count) {
        if (count) {
            *count = 0;
        }
        return;
    }

    for (uint32_t i = 0; i < g_world.capacity && found < max; i++) {
        if (g_world.entries[i].occupied &&
            g_world.entries[i].chunk &&
            g_world.entries[i].chunk->dirty_mask != 0) {
            if (out) {
                out[found] = g_world.entries[i].key;
            }
            found++;
        }
    }

    *count = found;
}

void mc_world_tick(tick_t current_tick)
{
    (void)current_tick;
    /* Placeholder: future work for block ticks, redstone, etc. */
}
