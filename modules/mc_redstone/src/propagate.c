/*
 * propagate.c -- BFS signal propagation for redstone wire.
 *
 * When a source activates, BFS outward through wire blocks.
 * Power decrements by 1 per wire block (from 15 down to 0).
 * Stops at non-wire blocks.
 */

#include "redstone_internal.h"
#include <string.h>

/* 6-directional neighbor offsets */
static const int32_t NEIGHBOR_OFFSETS[6][3] = {
    { 1,  0,  0},
    {-1,  0,  0},
    { 0,  1,  0},
    { 0, -1,  0},
    { 0,  0,  1},
    { 0,  0, -1},
};

#define BFS_CAPACITY     4096
#define VISITED_CAPACITY 4096

typedef struct {
    block_pos_t pos;
    uint8_t     power;
} bfs_entry_t;

typedef struct {
    block_pos_t pos;
    uint8_t     power;
    uint8_t     occupied;
} visited_entry_t;

/*
 * Returns 1 if already visited with equal or higher power, 0 otherwise.
 * On 0, records the position at the given power level.
 */
static int visited_check_and_set(visited_entry_t *table, block_pos_t pos, uint8_t power)
{
    uint32_t idx = rs_pos_hash(pos) % VISITED_CAPACITY;
    for (uint32_t i = 0; i < VISITED_CAPACITY; i++) {
        uint32_t slot = (idx + i) % VISITED_CAPACITY;
        if (!table[slot].occupied) {
            table[slot].pos      = pos;
            table[slot].power    = power;
            table[slot].occupied = 1;
            return 0;
        }
        if (rs_pos_equal(table[slot].pos, pos)) {
            if (table[slot].power >= power) {
                return 1;
            }
            table[slot].power = power;
            return 0;
        }
    }
    return 1; /* table full */
}

void mc_redstone_propagate_bfs(block_pos_t origin)
{
    mc_redstone_block_query_fn query_fn = mc_redstone_get_query_fn();
    if (!query_fn) {
        return;
    }

    block_id_t origin_block   = query_fn(origin);
    uint8_t    origin_is_src  = mc_redstone_is_power_source(origin_block);

    /* Determine initial power: 15 if origin or any neighbor is a source */
    uint8_t source_power = origin_is_src ? 15 : 0;

    if (!source_power) {
        for (int i = 0; i < 6; i++) {
            block_pos_t np = {
                origin.x + NEIGHBOR_OFFSETS[i][0],
                origin.y + NEIGHBOR_OFFSETS[i][1],
                origin.z + NEIGHBOR_OFFSETS[i][2],
                0
            };
            if (mc_redstone_is_power_source(query_fn(np))) {
                source_power = 15;
                break;
            }
        }
    }

    /* Set origin power */
    if (origin_block == BLOCK_REDSTONE_WIRE) {
        mc_redstone_power_map_set(origin, source_power);
    } else if (origin_is_src) {
        mc_redstone_power_map_set(origin, 15);
    }

    /* BFS from origin through wire blocks */
    static bfs_entry_t    queue[BFS_CAPACITY];
    static visited_entry_t visited[VISITED_CAPACITY];
    memset(visited, 0, sizeof(visited));

    uint32_t head = 0;
    uint32_t tail = 0;

    uint8_t seed_power = origin_is_src ? 15 : source_power;
    queue[tail].pos   = origin;
    queue[tail].power = seed_power;
    tail++;
    visited_check_and_set(visited, origin, seed_power);

    while (head < tail) {
        bfs_entry_t current = queue[head++];
        if (current.power == 0) {
            continue;
        }

        for (int i = 0; i < 6; i++) {
            block_pos_t np = {
                current.pos.x + NEIGHBOR_OFFSETS[i][0],
                current.pos.y + NEIGHBOR_OFFSETS[i][1],
                current.pos.z + NEIGHBOR_OFFSETS[i][2],
                0
            };

            if (query_fn(np) != BLOCK_REDSTONE_WIRE) {
                continue;
            }

            uint8_t new_power = current.power - 1;

            if (visited_check_and_set(visited, np, new_power)) {
                continue;
            }

            if (new_power > mc_redstone_power_map_get(np)) {
                mc_redstone_power_map_set(np, new_power);
            }

            if (new_power > 0 && tail < BFS_CAPACITY) {
                queue[tail].pos   = np;
                queue[tail].power = new_power;
                tail++;
            }
        }
    }
}
