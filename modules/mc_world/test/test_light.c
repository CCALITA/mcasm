/*
 * test_light.c -- Unit tests for light propagation in mc_world.
 *
 * Tests cover:
 *   - Block light BFS from a torch
 *   - Block light BFS from lava
 *   - Sky light column propagation
 *   - Opaque blocks blocking both sky and block light
 *   - No light in an unloaded chunk
 */

#include "mc_world.h"
#include "mc_block.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %-50s", #name); name(); printf("OK\n"); } while(0)

/* -----------------------------------------------------------
 * Helpers
 * --------------------------------------------------------- */

static void setup(void)
{
    mc_block_init();
    mc_world_init(42);
}

static void teardown(void)
{
    mc_world_shutdown();
    mc_block_shutdown();
}

/* -----------------------------------------------------------
 * Tests
 * --------------------------------------------------------- */

TEST(test_torch_block_light_adjacent)
{
    setup();

    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    /* Place torch at (8, 5, 8) -- center of chunk, section 0. */
    block_pos_t torch_pos = {8, 5, 8, 0};
    mc_world_set_block(torch_pos, BLOCK_TORCH);

    mc_world_propagate_light(cpos);

    /* Torch emits 14. The torch block itself should read 14. */
    assert(mc_world_get_block_light(torch_pos) == 14);

    /* Adjacent blocks (through air, transparent) should be 13. */
    block_pos_t adj_x = {9, 5, 8, 0};
    assert(mc_world_get_block_light(adj_x) == 13);

    block_pos_t adj_y = {8, 6, 8, 0};
    assert(mc_world_get_block_light(adj_y) == 13);

    block_pos_t adj_z = {8, 5, 9, 0};
    assert(mc_world_get_block_light(adj_z) == 13);

    /* Two steps away should be 12. */
    block_pos_t two_x = {10, 5, 8, 0};
    assert(mc_world_get_block_light(two_x) == 12);

    /* Three steps away should be 11. */
    block_pos_t three_x = {11, 5, 8, 0};
    assert(mc_world_get_block_light(three_x) == 11);

    teardown();
}

TEST(test_torch_block_light_falloff)
{
    setup();

    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    /* Place torch at (8, 5, 8). */
    block_pos_t torch_pos = {8, 5, 8, 0};
    mc_world_set_block(torch_pos, BLOCK_TORCH);
    mc_world_propagate_light(cpos);

    /* Walk along +x from the torch and verify decrease. */
    for (int dist = 0; dist <= 13; dist++) {
        block_pos_t bp = {8 + dist, 5, 8, 0};
        /* Clamped to chunk boundary (max x=15 for chunk 0). */
        if (bp.x > 15) break;
        uint8_t expected = (uint8_t)(14 - dist);
        uint8_t actual = mc_world_get_block_light(bp);
        assert(actual == expected);
    }

    teardown();
}

TEST(test_lava_light)
{
    setup();

    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    /* Lava emits 15. Place at (4, 2, 4). */
    block_pos_t lava_pos = {4, 2, 4, 0};
    mc_world_set_block(lava_pos, BLOCK_LAVA);
    mc_world_propagate_light(cpos);

    assert(mc_world_get_block_light(lava_pos) == 15);

    /* Lava is opaque (transparent=0), so light does not pass through it
     * via BFS. But the emitter itself is seeded at 15 and BFS starts from
     * it. Neighbors of an opaque emitter only get light if they are
     * transparent. Air is transparent, so adjacent air gets 14. */
    block_pos_t adj = {5, 2, 4, 0};
    assert(mc_world_get_block_light(adj) == 14);

    teardown();
}

TEST(test_opaque_block_stops_block_light)
{
    setup();

    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    /* Torch at (8, 5, 8). Stone wall at (9, 5, 8). */
    mc_world_set_block((block_pos_t){8, 5, 8, 0}, BLOCK_TORCH);
    mc_world_set_block((block_pos_t){9, 5, 8, 0}, BLOCK_STONE);
    mc_world_propagate_light(cpos);

    /* Stone itself gets no block light (opaque blocks don't receive). */
    assert(mc_world_get_block_light((block_pos_t){9, 5, 8, 0}) == 0);

    /* Block behind stone at (10, 5, 8) should not receive direct +x light
     * from torch, but it can receive light from other directions through
     * the BFS that goes around the stone. The path around is at least 3
     * steps (e.g., (8,5,8) -> (8,6,8) -> (9,6,8) -> (10,6,8) -> (10,5,8)).
     * That is 4 steps, so level = 14 - 4 = 10. */
    uint8_t behind = mc_world_get_block_light((block_pos_t){10, 5, 8, 0});
    assert(behind == 10);

    teardown();
}

TEST(test_sky_light_open_air)
{
    setup();

    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    /* No blocks placed -- entire chunk is air. */
    mc_world_propagate_light(cpos);

    /* Top of the world should have full skylight. */
    int max_y = SECTION_COUNT * CHUNK_SIZE_Y - 1;
    block_pos_t top = {8, max_y, 8, 0};
    assert(mc_world_get_sky_light(top) == 15);

    /* Ground level should also have 15 (column is all air). */
    block_pos_t ground = {8, 0, 8, 0};
    assert(mc_world_get_sky_light(ground) == 15);

    teardown();
}

TEST(test_sky_light_blocked_by_opaque)
{
    setup();

    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    /* Place a stone slab at y=100 across (8, z=8). */
    mc_world_set_block((block_pos_t){8, 100, 8, 0}, BLOCK_STONE);
    mc_world_propagate_light(cpos);

    /* Above the stone: sky light should be 15. */
    assert(mc_world_get_sky_light((block_pos_t){8, 101, 8, 0}) == 15);

    /* The stone itself: opaque, so 0 sky light. */
    assert(mc_world_get_sky_light((block_pos_t){8, 100, 8, 0}) == 0);

    /* Below the stone: no direct sky column, but BFS from neighboring
     * columns can spread in. The block at (8, 99, 8) is 2 steps from the
     * sky-lit block at (7, 99, 8) or (9, 99, 8) which gets 15 from column
     * pass, then (8, 99, 8) gets 15 - 1 = 14. */
    uint8_t below = mc_world_get_sky_light((block_pos_t){8, 99, 8, 0});
    assert(below == 14);

    teardown();
}

TEST(test_no_light_unloaded_chunk)
{
    setup();

    /* Don't load any chunk. */
    block_pos_t bp = {100, 50, 100, 0};
    assert(mc_world_get_block_light(bp) == 0);
    assert(mc_world_get_sky_light(bp) == 0);

    teardown();
}

TEST(test_glass_passes_sky_light)
{
    setup();

    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    /* Place glass at y=100. Glass is transparent, so sky light passes. */
    mc_world_set_block((block_pos_t){4, 100, 4, 0}, BLOCK_GLASS);
    mc_world_propagate_light(cpos);

    /* Glass should have sky light 15 (transparent in column pass). */
    assert(mc_world_get_sky_light((block_pos_t){4, 100, 4, 0}) == 15);

    /* Below glass should also be 15 (column continues through glass). */
    assert(mc_world_get_sky_light((block_pos_t){4, 99, 4, 0}) == 15);

    teardown();
}

TEST(test_propagate_light_null_chunk)
{
    setup();

    /* Calling propagate on a non-existent chunk should not crash. */
    chunk_pos_t missing = {999, 999};
    mc_world_propagate_light(missing);

    teardown();
}

/* -----------------------------------------------------------
 * Main
 * --------------------------------------------------------- */
int main(void)
{
    printf("mc_world light tests:\n");

    RUN(test_torch_block_light_adjacent);
    RUN(test_torch_block_light_falloff);
    RUN(test_lava_light);
    RUN(test_opaque_block_stops_block_light);
    RUN(test_sky_light_open_air);
    RUN(test_sky_light_blocked_by_opaque);
    RUN(test_no_light_unloaded_chunk);
    RUN(test_glass_passes_sky_light);
    RUN(test_propagate_light_null_chunk);

    printf("  ALL TESTS PASSED\n");
    return 0;
}
