/*
 * components.c -- Redstone component classification and tick scheduling.
 *
 * Identifies power sources, conductors, and components by block ID.
 * Processes scheduled ticks for delayed components (repeaters).
 */

#include "redstone_internal.h"

/* ---- Scheduled tick queue ---- */

static scheduled_tick_t g_tick_queue[MAX_SCHEDULED_TICKS];

/* ---- Component classification ---- */

uint8_t mc_redstone_is_power_source(block_id_t id)
{
    switch (id) {
    case BLOCK_REDSTONE_TORCH:
    case BLOCK_LEVER:
    case BLOCK_BUTTON:
        return 1;
    case BLOCK_REPEATER:
        /* Repeater is a source on its output face; simplified here. */
        return 1;
    default:
        return 0;
    }
}

uint8_t mc_redstone_is_conductor(block_id_t id)
{
    /*
     * Solid opaque blocks conduct power through them.
     * We treat the standard solid blocks as conductors.
     */
    switch (id) {
    case BLOCK_STONE:
    case BLOCK_GRASS:
    case BLOCK_DIRT:
    case BLOCK_COBBLESTONE:
    case BLOCK_OAK_PLANKS:
    case BLOCK_BEDROCK:
    case BLOCK_SAND:
    case BLOCK_GRAVEL:
    case BLOCK_OAK_LOG:
    case BLOCK_IRON_ORE:
    case BLOCK_COAL_ORE:
    case BLOCK_GOLD_ORE:
    case BLOCK_DIAMOND_ORE:
    case BLOCK_REDSTONE_ORE:
    case BLOCK_OBSIDIAN:
    case BLOCK_CRAFTING_TABLE:
    case BLOCK_FURNACE:
        return 1;
    default:
        return 0;
    }
}

uint8_t mc_redstone_is_component(block_id_t id)
{
    switch (id) {
    case BLOCK_REPEATER:
    case BLOCK_COMPARATOR:
    case BLOCK_PISTON:
        return 1;
    default:
        return 0;
    }
}

/* ---- Tick scheduling ---- */

void mc_redstone_schedule_tick(block_pos_t pos, tick_t fire_tick)
{
    for (uint32_t i = 0; i < MAX_SCHEDULED_TICKS; i++) {
        if (!g_tick_queue[i].active) {
            g_tick_queue[i].pos       = pos;
            g_tick_queue[i].fire_tick = fire_tick;
            g_tick_queue[i].active    = 1;
            return;
        }
    }
    /* Queue full -- drop the tick (best effort). */
}

void mc_redstone_tick(tick_t current_tick)
{
    for (uint32_t i = 0; i < MAX_SCHEDULED_TICKS; i++) {
        if (!g_tick_queue[i].active) {
            continue;
        }
        if (g_tick_queue[i].fire_tick <= current_tick) {
            /* Fire the scheduled update */
            block_pos_t pos = g_tick_queue[i].pos;
            g_tick_queue[i].active = 0;
            mc_redstone_update(pos);
        }
    }
}
