/*
 * test_worldgen.c -- Tests for mc_worldgen module.
 *
 * Verifies:
 *   1. Bedrock at y=0..4
 *   2. Stone below surface
 *   3. Grass at the surface
 *   4. Determinism: same seed + same position = identical chunk
 *   5. Biome API returns valid values
 *   6. Temperature and humidity are in [0,1]
 */

#include "mc_worldgen.h"
#include "mc_block.h"
#include "mc_types.h"
#include "mc_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_SEED 42

static int tests_run    = 0;
static int tests_passed = 0;

#define ASSERT(cond, msg)                                              \
    do {                                                               \
        tests_run++;                                                   \
        if (!(cond)) {                                                 \
            fprintf(stderr, "FAIL [%s:%d]: %s\n", __FILE__, __LINE__, \
                    msg);                                              \
            return 1;                                                  \
        }                                                              \
        tests_passed++;                                                \
    } while (0)

static block_id_t get_block(const chunk_t *c, int lx, int y, int lz)
{
    int sec = y / CHUNK_SIZE_Y;
    int ly  = y % CHUNK_SIZE_Y;
    return c->sections[sec].blocks[ly * 256 + lz * 16 + lx];
}

/* ------------------------------------------------------------------ */
/* Test: bedrock at y=0..4                                            */
/* ------------------------------------------------------------------ */
static int test_bedrock(void)
{
    chunk_t chunk;
    chunk_pos_t pos = {0, 0};
    mc_error_t err = mc_worldgen_generate_chunk(pos, &chunk);
    ASSERT(err == MC_OK, "generate_chunk should return MC_OK");

    for (int lx = 0; lx < CHUNK_SIZE_X; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE_Z; lz++) {
            for (int y = 0; y < 5; y++) {
                block_id_t b = get_block(&chunk, lx, y, lz);
                ASSERT(b == BLOCK_BEDROCK,
                       "expected bedrock below y=5");
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Test: stone below surface, grass at surface                        */
/* ------------------------------------------------------------------ */
static int test_surface_layers(void)
{
    chunk_t chunk;
    chunk_pos_t pos = {0, 0};
    mc_worldgen_generate_chunk(pos, &chunk);

    for (int lx = 0; lx < CHUNK_SIZE_X; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE_Z; lz++) {
            int surface = chunk.heightmap[lz * CHUNK_SIZE_X + lx];

            /* Surface block must be grass (or air if caves carved it). */
            block_id_t surf_block = get_block(&chunk, lx, surface, lz);
            ASSERT(surf_block == BLOCK_GRASS || surf_block == BLOCK_AIR,
                   "surface should be grass or air (cave)");

            /* Below bedrock layer, blocks should be stone or ore (unless caved). */
            if (surface > 10) {
                block_id_t deep = get_block(&chunk, lx, 8, lz);
                ASSERT(deep == BLOCK_STONE || deep == BLOCK_AIR ||
                       deep == BLOCK_COAL_ORE || deep == BLOCK_IRON_ORE ||
                       deep == BLOCK_GOLD_ORE || deep == BLOCK_DIAMOND_ORE ||
                       deep == BLOCK_REDSTONE_ORE,
                       "deep underground should be stone, ore, or air (cave)");
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Test: determinism (same seed + position = same chunk)              */
/* ------------------------------------------------------------------ */
static int test_determinism(void)
{
    chunk_t chunk_a, chunk_b;
    chunk_pos_t pos = {7, -3};

    mc_worldgen_init(TEST_SEED);
    mc_worldgen_generate_chunk(pos, &chunk_a);

    mc_worldgen_init(TEST_SEED);
    mc_worldgen_generate_chunk(pos, &chunk_b);

    /* Compare every section block-by-block. */
    for (int s = 0; s < SECTION_COUNT; s++) {
        int match = memcmp(chunk_a.sections[s].blocks,
                           chunk_b.sections[s].blocks,
                           sizeof(chunk_a.sections[s].blocks));
        ASSERT(match == 0, "chunks with same seed/pos must be identical");
    }

    /* Heightmaps must match. */
    int hm_match = memcmp(chunk_a.heightmap, chunk_b.heightmap,
                          sizeof(chunk_a.heightmap));
    ASSERT(hm_match == 0, "heightmaps must be identical for same seed/pos");

    return 0;
}

/* ------------------------------------------------------------------ */
/* Test: biome returns valid range                                    */
/* ------------------------------------------------------------------ */
static int test_biome_range(void)
{
    for (int x = -100; x <= 100; x += 17) {
        for (int z = -100; z <= 100; z += 17) {
            uint8_t b = mc_worldgen_get_biome(x, z);
            ASSERT(b < BIOME_COUNT, "biome id must be < BIOME_COUNT");
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Test: temperature and humidity in [0, 1]                           */
/* ------------------------------------------------------------------ */
static int test_temperature_humidity(void)
{
    for (int x = -200; x <= 200; x += 23) {
        for (int z = -200; z <= 200; z += 23) {
            float t = mc_worldgen_get_temperature(x, z);
            float h = mc_worldgen_get_humidity(x, z);
            ASSERT(t >= 0.0f && t <= 1.0f,
                   "temperature must be in [0,1]");
            ASSERT(h >= 0.0f && h <= 1.0f,
                   "humidity must be in [0,1]");
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Test: invalid argument handling                                    */
/* ------------------------------------------------------------------ */
static int test_null_arg(void)
{
    chunk_pos_t pos = {0, 0};
    mc_error_t err = mc_worldgen_generate_chunk(pos, NULL);
    ASSERT(err == MC_ERR_INVALID_ARG,
           "NULL out_chunk should return MC_ERR_INVALID_ARG");
    return 0;
}

/* ------------------------------------------------------------------ */
/* Test: chunk state is READY after generation                        */
/* ------------------------------------------------------------------ */
static int test_chunk_state(void)
{
    chunk_t chunk;
    chunk_pos_t pos = {1, 1};
    mc_worldgen_generate_chunk(pos, &chunk);
    ASSERT(chunk.state == CHUNK_STATE_READY,
           "chunk state should be READY after generation");
    ASSERT(chunk.pos.x == pos.x && chunk.pos.z == pos.z,
           "chunk position should match requested position");
    return 0;
}

/* ------------------------------------------------------------------ */
/* Test: ore generation places coal and iron ore blocks               */
/* ------------------------------------------------------------------ */
static int test_ore_generation(void)
{
    chunk_t chunk;
    chunk_pos_t pos = {3, 5};
    mc_error_t err = mc_worldgen_generate_chunk(pos, &chunk);
    ASSERT(err == MC_OK, "generate_chunk should return MC_OK");

    int coal_count = 0;
    int iron_count = 0;
    int max_y = SECTION_COUNT * CHUNK_SIZE_Y;

    for (int lx = 0; lx < CHUNK_SIZE_X; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE_Z; lz++) {
            for (int y = 0; y < max_y; y++) {
                block_id_t b = get_block(&chunk, lx, y, lz);
                if (b == BLOCK_COAL_ORE)     coal_count++;
                if (b == BLOCK_IRON_ORE)     iron_count++;
            }
        }
    }

    ASSERT(coal_count > 0, "chunk should contain at least some coal ore");
    ASSERT(iron_count > 0, "chunk should contain at least some iron ore");

    return 0;
}

/* ------------------------------------------------------------------ */
/* Test: ores only replace stone, not air or other blocks             */
/* ------------------------------------------------------------------ */
static int test_ore_replaces_only_stone(void)
{
    /* Generate a chunk and verify no ore appears above the surface
       (since ores only replace stone, not air). */
    chunk_t chunk;
    chunk_pos_t pos = {0, 0};
    mc_worldgen_generate_chunk(pos, &chunk);

    int max_y = SECTION_COUNT * CHUNK_SIZE_Y;
    for (int lx = 0; lx < CHUNK_SIZE_X; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE_Z; lz++) {
            int surface = chunk.heightmap[lz * CHUNK_SIZE_X + lx];
            /* Above surface should still be air (ores don't create blocks in air). */
            for (int y = surface + 1; y < max_y; y++) {
                block_id_t b = get_block(&chunk, lx, y, lz);
                /* Allow air, or tree blocks placed above surface */
                if (b == BLOCK_COAL_ORE || b == BLOCK_IRON_ORE ||
                    b == BLOCK_GOLD_ORE || b == BLOCK_DIAMOND_ORE ||
                    b == BLOCK_REDSTONE_ORE) {
                    ASSERT(0, "ore should not appear above the surface in air");
                }
            }
        }
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */
int main(void)
{
    mc_worldgen_init(TEST_SEED);

    int fail = 0;
    fail |= test_bedrock();
    fail |= test_surface_layers();
    fail |= test_determinism();
    fail |= test_biome_range();
    fail |= test_temperature_humidity();
    fail |= test_null_arg();
    fail |= test_chunk_state();
    fail |= test_ore_generation();
    fail |= test_ore_replaces_only_stone();

    mc_worldgen_shutdown();

    printf("worldgen: %d/%d tests passed\n", tests_passed, tests_run);
    return fail ? 1 : 0;
}
