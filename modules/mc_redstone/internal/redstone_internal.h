/*
 * redstone_internal.h -- Internal declarations shared between redstone.c,
 * propagate (C or ASM), and components.c.  Never include from outside this
 * module.
 */

#ifndef REDSTONE_INTERNAL_H
#define REDSTONE_INTERNAL_H

#include "mc_redstone.h"
#include "mc_block.h"

/* Shared position hashing (used by power map and BFS visited set) */
static inline uint32_t rs_pos_hash(block_pos_t p)
{
    uint32_t h = (uint32_t)p.x * 73856093u;
    h ^= (uint32_t)p.y * 19349663u;
    h ^= (uint32_t)p.z * 83492791u;
    return h;
}

static inline int rs_pos_equal(block_pos_t a, block_pos_t b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

/* Power-map accessors (defined in redstone.c) */
void    mc_redstone_power_map_set(block_pos_t pos, uint8_t level);
uint8_t mc_redstone_power_map_get(block_pos_t pos);

/* DI callback accessor (defined in redstone.c) */
mc_redstone_block_query_fn mc_redstone_get_query_fn(void);

/* BFS propagation entry point (defined in propagate.c or propagate.asm) */
void mc_redstone_propagate_bfs(block_pos_t origin);

/* Scheduled tick entry (defined in components.c) */
typedef struct {
    block_pos_t pos;
    tick_t      fire_tick;
    uint8_t     active;
} scheduled_tick_t;

#define MAX_SCHEDULED_TICKS 256

#endif /* REDSTONE_INTERNAL_H */
