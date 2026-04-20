#include "mc_world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %-50s", #name); name(); printf("OK\n"); } while(0)

/* -----------------------------------------------------------
 * Helper: initialize a fresh world before each test
 * --------------------------------------------------------- */
static void setup(void)
{
    mc_world_init(12345);
}

static void teardown(void)
{
    mc_world_shutdown();
}

/* -----------------------------------------------------------
 * Tests
 * --------------------------------------------------------- */

TEST(test_init_and_shutdown)
{
    setup();
    assert(mc_world_loaded_chunk_count() == 0);
    teardown();
}

TEST(test_load_chunk)
{
    setup();
    chunk_pos_t pos = {0, 0};
    mc_error_t err = mc_world_load_chunk(pos);
    assert(err == MC_OK);
    assert(mc_world_loaded_chunk_count() == 1);

    const chunk_t *c = mc_world_get_chunk(pos);
    assert(c != NULL);
    assert(c->pos.x == 0 && c->pos.z == 0);
    teardown();
}

TEST(test_load_duplicate_chunk)
{
    setup();
    chunk_pos_t pos = {3, 7};
    assert(mc_world_load_chunk(pos) == MC_OK);
    assert(mc_world_load_chunk(pos) == MC_ERR_ALREADY_EXISTS);
    assert(mc_world_loaded_chunk_count() == 1);
    teardown();
}

TEST(test_unload_chunk)
{
    setup();
    chunk_pos_t pos = {1, 2};
    mc_world_load_chunk(pos);
    assert(mc_world_loaded_chunk_count() == 1);
    mc_world_unload_chunk(pos);
    assert(mc_world_loaded_chunk_count() == 0);
    assert(mc_world_get_chunk(pos) == NULL);
    teardown();
}

TEST(test_fill_section_and_get_block)
{
    setup();
    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    /* Fill section 0 with a known pattern */
    block_id_t blocks[BLOCKS_PER_SECTION];
    for (int i = 0; i < BLOCKS_PER_SECTION; i++) {
        blocks[i] = (block_id_t)(i % 256 + 1);  /* non-zero */
    }

    mc_error_t err = mc_world_fill_section(cpos, 0, blocks);
    assert(err == MC_OK);

    /* Verify get_block returns correct values for blocks in section 0 */
    /* Block at world (0,0,0) -> chunk(0,0), section 0, local(0,0,0), index 0 */
    block_pos_t bp0 = {0, 0, 0, 0};
    assert(mc_world_get_block(bp0) == blocks[0]);

    /* Block at world (5,3,7) -> chunk(0,0), section 0, local(5,3,7)
     * index = 3*256 + 7*16 + 5 = 768 + 112 + 5 = 885 */
    block_pos_t bp1 = {5, 3, 7, 0};
    assert(mc_world_get_block(bp1) == blocks[885]);

    /* Block at world (15,15,15) -> chunk(0,0), section 0, local(15,15,15)
     * index = 15*256 + 15*16 + 15 = 3840+240+15 = 4095 */
    block_pos_t bp2 = {15, 15, 15, 0};
    assert(mc_world_get_block(bp2) == blocks[4095]);

    teardown();
}

TEST(test_set_block_marks_dirty)
{
    setup();
    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    const chunk_t *chunk = mc_world_get_chunk(cpos);
    assert(chunk->dirty_mask == 0);

    block_pos_t bp = {2, 5, 3, 0};  /* section 0 */
    mc_error_t err = mc_world_set_block(bp, 42);
    assert(err == MC_OK);

    /* Re-fetch (pointer is stable) */
    chunk = mc_world_get_chunk(cpos);
    assert(chunk->dirty_mask & 1u);  /* section 0 dirty */

    /* Verify the block was actually set */
    assert(mc_world_get_block(bp) == 42);

    teardown();
}

TEST(test_set_block_updates_non_air_count)
{
    setup();
    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    const chunk_t *chunk = mc_world_get_chunk(cpos);
    assert(chunk->sections[0].non_air_count == 0);

    /* Place a block */
    block_pos_t bp = {0, 0, 0, 0};
    mc_world_set_block(bp, 1);
    assert(chunk->sections[0].non_air_count == 1);

    /* Remove it */
    mc_world_set_block(bp, 0);
    assert(chunk->sections[0].non_air_count == 0);

    teardown();
}

TEST(test_get_dirty_chunks)
{
    setup();
    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    uint32_t count = 0;
    chunk_pos_t dirty[8];
    mc_world_get_dirty_chunks(dirty, 8, &count);
    assert(count == 0);

    /* Dirty it */
    block_pos_t bp = {1, 1, 1, 0};
    mc_world_set_block(bp, 7);

    mc_world_get_dirty_chunks(dirty, 8, &count);
    assert(count == 1);
    assert(dirty[0].x == 0 && dirty[0].z == 0);

    teardown();
}

TEST(test_multiple_chunks)
{
    setup();
    chunk_pos_t p1 = {0, 0};
    chunk_pos_t p2 = {1, 0};
    chunk_pos_t p3 = {-1, -1};

    assert(mc_world_load_chunk(p1) == MC_OK);
    assert(mc_world_load_chunk(p2) == MC_OK);
    assert(mc_world_load_chunk(p3) == MC_OK);
    assert(mc_world_loaded_chunk_count() == 3);

    /* Set blocks in different chunks */
    block_pos_t bp_c1 = {5, 10, 5, 0};      /* chunk (0,0) */
    block_pos_t bp_c2 = {20, 10, 5, 0};     /* chunk (1,0) */
    block_pos_t bp_c3 = {-5, 10, -5, 0};    /* chunk (-1,-1) */

    mc_world_set_block(bp_c1, 10);
    mc_world_set_block(bp_c2, 20);
    mc_world_set_block(bp_c3, 30);

    assert(mc_world_get_block(bp_c1) == 10);
    assert(mc_world_get_block(bp_c2) == 20);
    assert(mc_world_get_block(bp_c3) == 30);

    /* Unload middle chunk */
    mc_world_unload_chunk(p2);
    assert(mc_world_loaded_chunk_count() == 2);
    assert(mc_world_get_block(bp_c2) == 0);  /* no chunk -> air */

    /* Other chunks still work */
    assert(mc_world_get_block(bp_c1) == 10);
    assert(mc_world_get_block(bp_c3) == 30);

    teardown();
}

TEST(test_negative_coords)
{
    setup();
    /* Chunk at (-1,-1) covers world x [-16..-1], z [-16..-1] */
    chunk_pos_t cpos = {-1, -1};
    mc_world_load_chunk(cpos);

    block_pos_t bp = {-1, 0, -1, 0};
    mc_world_set_block(bp, 99);
    assert(mc_world_get_block(bp) == 99);

    /* (-1) & 15 == 15, so local = (15, 0, 15), index = 0*256 + 15*16 + 15 = 255 */
    block_pos_t bp2 = {-16, 0, -16, 0};
    mc_world_set_block(bp2, 77);
    assert(mc_world_get_block(bp2) == 77);

    teardown();
}

TEST(test_out_of_range_y)
{
    setup();
    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    /* y = -1 is below section 0 */
    block_pos_t bp_neg = {0, -1, 0, 0};
    assert(mc_world_get_block(bp_neg) == 0);

    /* y = SECTION_COUNT * 16 is above the world */
    block_pos_t bp_high = {0, SECTION_COUNT * 16, 0, 0};
    assert(mc_world_get_block(bp_high) == 0);

    teardown();
}

TEST(test_heightmap)
{
    setup();
    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    /* Heightmap defaults to 0 since chunk is calloc'd */
    assert(mc_world_get_height(0, 0) == 0);

    /* No chunk loaded at (100,100) */
    assert(mc_world_get_height(100, 100) == 0);

    teardown();
}

TEST(test_fill_section_non_air_count)
{
    setup();
    chunk_pos_t cpos = {0, 0};
    mc_world_load_chunk(cpos);

    block_id_t blocks[BLOCKS_PER_SECTION];
    memset(blocks, 0, sizeof(blocks));
    /* Set exactly 100 blocks to non-air */
    for (int i = 0; i < 100; i++) {
        blocks[i] = 1;
    }

    mc_world_fill_section(cpos, 0, blocks);

    const chunk_t *chunk = mc_world_get_chunk(cpos);
    assert(chunk->sections[0].non_air_count == 100);

    teardown();
}

TEST(test_many_chunks_stress)
{
    setup();
    /* Load 200 chunks to trigger hash map growth */
    for (int x = -10; x < 10; x++) {
        for (int z = -10; z < 10; z++) {
            chunk_pos_t pos = {x, z};
            assert(mc_world_load_chunk(pos) == MC_OK);
        }
    }
    assert(mc_world_loaded_chunk_count() == 400);

    /* Verify all are findable */
    for (int x = -10; x < 10; x++) {
        for (int z = -10; z < 10; z++) {
            chunk_pos_t pos = {x, z};
            const chunk_t *c = mc_world_get_chunk(pos);
            assert(c != NULL);
            assert(c->pos.x == x && c->pos.z == z);
        }
    }

    teardown();
}

/* -----------------------------------------------------------
 * Main
 * --------------------------------------------------------- */
int main(void)
{
    printf("mc_world tests:\n");

    RUN(test_init_and_shutdown);
    RUN(test_load_chunk);
    RUN(test_load_duplicate_chunk);
    RUN(test_unload_chunk);
    RUN(test_fill_section_and_get_block);
    RUN(test_set_block_marks_dirty);
    RUN(test_set_block_updates_non_air_count);
    RUN(test_get_dirty_chunks);
    RUN(test_multiple_chunks);
    RUN(test_negative_coords);
    RUN(test_out_of_range_y);
    RUN(test_heightmap);
    RUN(test_fill_section_non_air_count);
    RUN(test_many_chunks_stress);

    printf("  ALL TESTS PASSED\n");
    return 0;
}
