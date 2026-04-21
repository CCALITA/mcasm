/*
 * ores.c -- Ore vein placement within generated chunks.
 *
 * Places ore veins at appropriate Y levels after terrain and caves
 * are generated but before trees.  Each ore type has a Y range,
 * vein size range, and number of veins per chunk.
 *
 * Vein algorithm:
 *   1. Pick a random position in the chunk (seeded from chunk_pos + ore_type).
 *   2. Check Y is within the ore's allowed range.
 *   3. Place ore blocks in a small 3D cluster around the center.
 *   4. Only replace BLOCK_STONE (preserves air, dirt, bedrock, etc.).
 */

#include "worldgen_internal.h"

/* ------------------------------------------------------------------ */
/* Simple seeded RNG (LCG) for deterministic ore placement            */
/* ------------------------------------------------------------------ */

typedef struct {
    uint32_t state;
} ore_rng_t;

static inline uint32_t ore_rng_next(ore_rng_t *rng)
{
    rng->state = rng->state * 1103515245u + 12345u;
    return (rng->state >> 16) & 0x7FFF;
}

/* Return a random int in [lo, hi] inclusive. */
static inline int ore_rng_range(ore_rng_t *rng, int lo, int hi)
{
    if (lo >= hi) {
        return lo;
    }
    return lo + (int)(ore_rng_next(rng) % (uint32_t)(hi - lo + 1));
}

/* ------------------------------------------------------------------ */
/* Ore type definitions                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    block_id_t block;
    int        y_min;
    int        y_max;
    int        vein_min;
    int        vein_max;
    int        veins_per_chunk;
    uint32_t   seed_salt;
} ore_def_t;

static const ore_def_t ORE_DEFS[] = {
    { BLOCK_COAL_ORE,     0, 128, 5, 12, 20, 0x10001 },
    { BLOCK_IRON_ORE,     0,  64, 3,  8, 15, 0x20002 },
    { BLOCK_GOLD_ORE,     0,  32, 3,  6,  5, 0x30003 },
    { BLOCK_DIAMOND_ORE,  0,  16, 2,  5,  2, 0x40004 },
    { BLOCK_REDSTONE_ORE, 0,  16, 3,  8,  8, 0x50005 },
};

#define ORE_DEF_COUNT ((int)(sizeof(ORE_DEFS) / sizeof(ORE_DEFS[0])))

/* ------------------------------------------------------------------ */
/* Block replacement helper                                           */
/* ------------------------------------------------------------------ */

/* Replace a block with ore only if the current block is BLOCK_STONE. */
static inline void chunk_replace_stone(chunk_t *c, int lx, int y, int lz, block_id_t ore)
{
    if (lx < 0 || lx >= CHUNK_SIZE_X || lz < 0 || lz >= CHUNK_SIZE_Z) {
        return;
    }
    if (y < 0 || y >= SECTION_COUNT * CHUNK_SIZE_Y) {
        return;
    }
    int sec_idx = y / CHUNK_SIZE_Y;
    int local_y = y % CHUNK_SIZE_Y;
    int index   = local_y * 256 + lz * 16 + lx;
    chunk_section_t *sec = &c->sections[sec_idx];
    if (sec->blocks[index] == BLOCK_STONE) {
        sec->blocks[index] = ore;
    }
}

/* ------------------------------------------------------------------ */
/* Place a single vein as a 3D cluster around a center point          */
/* ------------------------------------------------------------------ */

static void place_vein(chunk_t *chunk, ore_rng_t *rng,
                       int cx, int cy, int cz,
                       block_id_t ore, int vein_size)
{
    chunk_replace_stone(chunk, cx, cy, cz, ore);

    int x = cx;
    int y = cy;
    int z = cz;

    for (int i = 1; i < vein_size; i++) {
        int dir = ore_rng_range(rng, 0, 5);
        switch (dir) {
            case 0: x++; break;
            case 1: x--; break;
            case 2: y++; break;
            case 3: y--; break;
            case 4: z++; break;
            case 5: z--; break;
        }
        chunk_replace_stone(chunk, x, y, z, ore);
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */

void ores_init(uint32_t seed)
{
    (void)seed;
}

void ores_place(chunk_pos_t pos, chunk_t *chunk)
{
    int max_y = SECTION_COUNT * CHUNK_SIZE_Y - 1;

    for (int i = 0; i < ORE_DEF_COUNT; i++) {
        const ore_def_t *def = &ORE_DEFS[i];

        /* Clamp y_max to actual chunk height. */
        int y_max = def->y_max;
        if (y_max > max_y) {
            y_max = max_y;
        }

        for (int v = 0; v < def->veins_per_chunk; v++) {
            /* Seed RNG from chunk position, ore type, and vein index. */
            uint32_t seed_val = (uint32_t)pos.x * 73856093u
                              + (uint32_t)pos.z * 19349663u
                              + def->seed_salt
                              + (uint32_t)v * 7919u;
            ore_rng_t rng = { seed_val };

            int lx = ore_rng_range(&rng, 0, CHUNK_SIZE_X - 1);
            int lz = ore_rng_range(&rng, 0, CHUNK_SIZE_Z - 1);
            int y  = ore_rng_range(&rng, def->y_min, y_max);

            int vein_size = ore_rng_range(&rng, def->vein_min, def->vein_max);

            place_vein(chunk, &rng, lx, y, lz, def->block, vein_size);
        }
    }
}
