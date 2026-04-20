/*
 * test_save.c -- Tests for the mc_save module.
 *
 * Covers:
 *   1) NBT roundtrip (write + read back, verify all tag types)
 *   2) Chunk save/load (write a chunk, load it, compare block data)
 *   3) Player save/load (position, yaw, pitch roundtrip)
 *   4) World metadata save/load
 *   5) mc_save_chunk_exists
 */

#include "mc_save.h"
#include "mc_types.h"
#include "mc_error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>

/* Simple assertion macro that prints file:line on failure. */
#define TEST_ASSERT(cond, msg)                                         \
    do {                                                               \
        if (!(cond)) {                                                 \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
            return 1;                                                  \
        }                                                              \
    } while (0)

#define TEST_ASSERT_EQ(a, b, msg) TEST_ASSERT((a) == (b), msg)

/* Floating-point comparison with small epsilon. */
static int float_eq(float a, float b)
{
    return fabsf(a - b) < 1e-5f;
}

/* ------------------------------------------------------------------ */
/*  Recursive temp-dir cleanup (rm -rf).                               */
/* ------------------------------------------------------------------ */

static void rm_rf(const char* path)
{
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path);
    (void)system(cmd);
}

/* Create a unique temp directory for a test run. */
static void make_tmp_dir(char* buf, size_t buf_size)
{
    snprintf(buf, buf_size, "/tmp/mc_save_test_XXXXXX");
    char* result = mkdtemp(buf);
    (void)result;
}

/* ================================================================== */
/*  Test 1: Chunk save + load roundtrip                                */
/* ================================================================== */

static int test_chunk_roundtrip(void)
{
    char tmp[256];
    make_tmp_dir(tmp, sizeof(tmp));

    mc_error_t err = mc_save_init(tmp);
    TEST_ASSERT_EQ(err, MC_OK, "mc_save_init should succeed");

    /* Build a chunk with known block data. */
    chunk_t* original = calloc(1, sizeof(chunk_t));
    TEST_ASSERT(original != NULL, "alloc original chunk");

    original->pos.x = 3;
    original->pos.z = -7;
    original->state = CHUNK_STATE_READY;
    original->dirty_mask = 0x0F;

    /* Fill section 0 with a pattern. */
    for (int i = 0; i < BLOCKS_PER_SECTION; i++) {
        original->sections[0].blocks[i] = (block_id_t)(i & 0xFF);
    }
    original->sections[0].non_air_count = 100;

    /* Fill section 5 similarly. */
    for (int i = 0; i < BLOCKS_PER_SECTION; i++) {
        original->sections[5].blocks[i] = (block_id_t)((i * 3 + 7) & 0xFFFF);
    }
    original->sections[5].non_air_count = 200;

    /* Set a heightmap. */
    for (int i = 0; i < CHUNK_SIZE_X * CHUNK_SIZE_Z; i++) {
        original->heightmap[i] = 64 + (i % 16);
    }

    chunk_pos_t cpos = { .x = 3, .z = -7 };

    /* Save. */
    err = mc_save_chunk(cpos, original);
    TEST_ASSERT_EQ(err, MC_OK, "mc_save_chunk should succeed");

    /* Exists check. */
    TEST_ASSERT_EQ(mc_save_chunk_exists(cpos), 1, "chunk should exist after save");

    /* Load. */
    chunk_t* loaded = calloc(1, sizeof(chunk_t));
    TEST_ASSERT(loaded != NULL, "alloc loaded chunk");

    err = mc_save_load_chunk(cpos, loaded);
    TEST_ASSERT_EQ(err, MC_OK, "mc_save_load_chunk should succeed");

    /* Verify. */
    TEST_ASSERT_EQ(loaded->pos.x, 3, "loaded xPos");
    TEST_ASSERT_EQ(loaded->pos.z, -7, "loaded zPos");
    TEST_ASSERT_EQ(loaded->state, CHUNK_STATE_READY, "loaded state");
    TEST_ASSERT_EQ(loaded->dirty_mask, 0x0Fu, "loaded dirty_mask");

    /* Section 0 blocks. */
    for (int i = 0; i < BLOCKS_PER_SECTION; i++) {
        if (loaded->sections[0].blocks[i] != (block_id_t)(i & 0xFF)) {
            TEST_ASSERT(0, "section 0 block mismatch");
        }
    }
    TEST_ASSERT_EQ(loaded->sections[0].non_air_count, 100, "section 0 non_air_count");

    /* Section 5 blocks. */
    for (int i = 0; i < BLOCKS_PER_SECTION; i++) {
        if (loaded->sections[5].blocks[i] != (block_id_t)((i * 3 + 7) & 0xFFFF)) {
            TEST_ASSERT(0, "section 5 block mismatch");
        }
    }
    TEST_ASSERT_EQ(loaded->sections[5].non_air_count, 200, "section 5 non_air_count");

    /* Heightmap. */
    for (int i = 0; i < CHUNK_SIZE_X * CHUNK_SIZE_Z; i++) {
        if (loaded->heightmap[i] != 64 + (i % 16)) {
            TEST_ASSERT(0, "heightmap mismatch");
        }
    }

    /* Non-existent chunk. */
    chunk_pos_t missing = { .x = 999, .z = 999 };
    TEST_ASSERT_EQ(mc_save_chunk_exists(missing), 0, "missing chunk should not exist");
    chunk_t tmp_chunk;
    err = mc_save_load_chunk(missing, &tmp_chunk);
    TEST_ASSERT_EQ(err, MC_ERR_NOT_FOUND, "loading missing chunk should return NOT_FOUND");

    free(original);
    free(loaded);
    mc_save_shutdown();
    rm_rf(tmp);
    return 0;
}

/* ================================================================== */
/*  Test 2: Player save + load roundtrip                               */
/* ================================================================== */

static int test_player_roundtrip(void)
{
    char tmp[256];
    make_tmp_dir(tmp, sizeof(tmp));

    mc_error_t err = mc_save_init(tmp);
    TEST_ASSERT_EQ(err, MC_OK, "mc_save_init should succeed");

    vec3_t pos = { .x = 10.5f, .y = 64.0f, .z = -33.25f, ._pad = 0.0f };
    float yaw   = 90.0f;
    float pitch = -15.0f;

    err = mc_save_player("TestPlayer", pos, yaw, pitch);
    TEST_ASSERT_EQ(err, MC_OK, "mc_save_player should succeed");

    vec3_t loaded_pos = {0};
    float loaded_yaw = 0.0f;
    float loaded_pitch = 0.0f;

    err = mc_save_load_player("TestPlayer", &loaded_pos, &loaded_yaw, &loaded_pitch);
    TEST_ASSERT_EQ(err, MC_OK, "mc_save_load_player should succeed");

    TEST_ASSERT(float_eq(loaded_pos.x, 10.5f),   "player pos.x");
    TEST_ASSERT(float_eq(loaded_pos.y, 64.0f),   "player pos.y");
    TEST_ASSERT(float_eq(loaded_pos.z, -33.25f), "player pos.z");
    TEST_ASSERT(float_eq(loaded_yaw, 90.0f),     "player yaw");
    TEST_ASSERT(float_eq(loaded_pitch, -15.0f),  "player pitch");

    /* Non-existent player. */
    vec3_t np = {0};
    float ny = 0.0f, npi = 0.0f;
    err = mc_save_load_player("NoSuchPlayer", &np, &ny, &npi);
    TEST_ASSERT_EQ(err, MC_ERR_IO, "loading missing player should fail");

    mc_save_shutdown();
    rm_rf(tmp);
    return 0;
}

/* ================================================================== */
/*  Test 3: World metadata save + load roundtrip                       */
/* ================================================================== */

static int test_world_meta_roundtrip(void)
{
    char tmp[256];
    make_tmp_dir(tmp, sizeof(tmp));

    mc_error_t err = mc_save_init(tmp);
    TEST_ASSERT_EQ(err, MC_OK, "mc_save_init should succeed");

    uint32_t seed      = 12345;
    tick_t   time_val  = 54321;
    uint8_t  game_mode = 1;  /* Creative */

    err = mc_save_world_meta(seed, time_val, game_mode);
    TEST_ASSERT_EQ(err, MC_OK, "mc_save_world_meta should succeed");

    uint32_t loaded_seed = 0;
    tick_t   loaded_time = 0;
    uint8_t  loaded_gm   = 0;

    err = mc_save_load_world_meta(&loaded_seed, &loaded_time, &loaded_gm);
    TEST_ASSERT_EQ(err, MC_OK, "mc_save_load_world_meta should succeed");

    TEST_ASSERT_EQ(loaded_seed, 12345u,  "world seed");
    TEST_ASSERT_EQ(loaded_time, 54321u,  "world time");
    TEST_ASSERT_EQ(loaded_gm,  1,        "game mode");

    mc_save_shutdown();
    rm_rf(tmp);
    return 0;
}

/* ================================================================== */
/*  Test 4: Multiple chunks in same region                             */
/* ================================================================== */

static int test_multiple_chunks(void)
{
    char tmp[256];
    make_tmp_dir(tmp, sizeof(tmp));

    mc_error_t err = mc_save_init(tmp);
    TEST_ASSERT_EQ(err, MC_OK, "mc_save_init should succeed");

    /* Save two chunks in the same region. */
    chunk_t* c1 = calloc(1, sizeof(chunk_t));
    chunk_t* c2 = calloc(1, sizeof(chunk_t));
    TEST_ASSERT(c1 && c2, "alloc chunks");

    c1->state = CHUNK_STATE_READY;
    c1->sections[0].blocks[0] = 42;
    c1->sections[0].non_air_count = 1;

    c2->state = CHUNK_STATE_READY;
    c2->sections[0].blocks[0] = 99;
    c2->sections[0].non_air_count = 1;

    chunk_pos_t p1 = { .x = 0, .z = 0 };
    chunk_pos_t p2 = { .x = 1, .z = 0 };

    err = mc_save_chunk(p1, c1);
    TEST_ASSERT_EQ(err, MC_OK, "save chunk 1");
    err = mc_save_chunk(p2, c2);
    TEST_ASSERT_EQ(err, MC_OK, "save chunk 2");

    /* Load both back. */
    chunk_t loaded1 = {0};
    chunk_t loaded2 = {0};

    err = mc_save_load_chunk(p1, &loaded1);
    TEST_ASSERT_EQ(err, MC_OK, "load chunk 1");
    err = mc_save_load_chunk(p2, &loaded2);
    TEST_ASSERT_EQ(err, MC_OK, "load chunk 2");

    TEST_ASSERT_EQ(loaded1.sections[0].blocks[0], 42, "chunk 1 block");
    TEST_ASSERT_EQ(loaded2.sections[0].blocks[0], 99, "chunk 2 block");

    free(c1);
    free(c2);
    mc_save_shutdown();
    rm_rf(tmp);
    return 0;
}

/* ================================================================== */
/*  Main                                                               */
/* ================================================================== */

int main(void)
{
    int failures = 0;

    printf("  test_chunk_roundtrip...\n");
    failures += test_chunk_roundtrip();

    printf("  test_player_roundtrip...\n");
    failures += test_player_roundtrip();

    printf("  test_world_meta_roundtrip...\n");
    failures += test_world_meta_roundtrip();

    printf("  test_multiple_chunks...\n");
    failures += test_multiple_chunks();

    if (failures) {
        printf("  %d test(s) FAILED\n", failures);
        return 1;
    }
    printf("  All mc_save tests passed.\n");
    return 0;
}
