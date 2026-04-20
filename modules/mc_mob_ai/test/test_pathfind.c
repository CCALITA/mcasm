#include "mc_mob_ai.h"
#include "mc_block.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Test infrastructure ──────────────────────────────────────────── */

static int tests_run    = 0;
static int tests_passed = 0;

#define TEST(name) static void name(void)
#define RUN(name) do { \
    printf("  %-50s", #name); \
    tests_run++; \
    name(); \
    tests_passed++; \
    printf("PASS\n"); \
} while (0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n    %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        exit(1); \
    } \
} while (0)

/* ── Mock block world ─────────────────────────────────────────────── */

/*
 * Flat terrain: y < 4 is BLOCK_STONE, y >= 4 is BLOCK_AIR.
 * Optionally place a wall for unreachable tests.
 */

#define WORLD_SIZE 64
static uint8_t g_wall_enabled = 0;

static block_id_t mock_block_query(block_pos_t pos)
{
    /* Out of bounds → stone (impassable) */
    if (pos.x < 0 || pos.x >= WORLD_SIZE ||
        pos.z < 0 || pos.z >= WORLD_SIZE ||
        pos.y < 0 || pos.y >= WORLD_SIZE) {
        return BLOCK_STONE;
    }

    /* Wall at x=5, for all y >= 4 and all z — blocks passage */
    if (g_wall_enabled && pos.x == 5 && pos.y >= 4) {
        return BLOCK_STONE;
    }

    /* Flat ground at y < 4 */
    if (pos.y < 4) {
        return BLOCK_STONE;
    }
    return BLOCK_AIR;
}

/* ── Pathfinding tests ────────────────────────────────────────────── */

TEST(test_pathfind_flat_terrain)
{
    g_wall_enabled = 0;
    mc_mob_ai_init(mock_block_query);

    vec3_t start = {2.0f, 4.0f, 2.0f, 0.0f};
    vec3_t end   = {8.0f, 4.0f, 2.0f, 0.0f};

    vec3_t path[256];
    uint32_t length = 0;

    uint8_t found = mc_mob_ai_pathfind(start, end, path, 1000, &length);

    ASSERT(found == 1);
    ASSERT(length > 0);

    /* Path should start at (2,4,2) and end at (8,4,2) */
    ASSERT(fabsf(path[0].x - 2.0f) < 0.5f);
    ASSERT(fabsf(path[0].y - 4.0f) < 0.5f);
    ASSERT(fabsf(path[0].z - 2.0f) < 0.5f);

    ASSERT(fabsf(path[length - 1].x - 8.0f) < 0.5f);
    ASSERT(fabsf(path[length - 1].y - 4.0f) < 0.5f);
    ASSERT(fabsf(path[length - 1].z - 2.0f) < 0.5f);

    mc_mob_ai_shutdown();
}

TEST(test_pathfind_unreachable)
{
    g_wall_enabled = 1;
    mc_mob_ai_init(mock_block_query);

    /* Start on one side of the wall, end on the other.
       Wall at x=5 (full height), so (2,4,2) → (8,4,2) is blocked.
       Wall extends across all z, so no way around in the z plane either
       (mock returns STONE for y>=4 at x=5, and STONE for y<4 everywhere,
       so there is no walkable path through or around). */
    vec3_t start = {2.0f, 4.0f, 2.0f, 0.0f};
    vec3_t end   = {8.0f, 4.0f, 2.0f, 0.0f};

    vec3_t path[256];
    uint32_t length = 0;

    uint8_t found = mc_mob_ai_pathfind(start, end, path, 500, &length);

    ASSERT(found == 0);
    ASSERT(length == 0);

    g_wall_enabled = 0;
    mc_mob_ai_shutdown();
}

TEST(test_pathfind_same_position)
{
    g_wall_enabled = 0;
    mc_mob_ai_init(mock_block_query);

    vec3_t start = {5.0f, 4.0f, 5.0f, 0.0f};
    vec3_t end   = {5.0f, 4.0f, 5.0f, 0.0f};

    vec3_t path[256];
    uint32_t length = 0;

    uint8_t found = mc_mob_ai_pathfind(start, end, path, 100, &length);

    /* Should find trivial path (just the start/end node) */
    ASSERT(found == 1);
    ASSERT(length >= 1);

    mc_mob_ai_shutdown();
}

/* ── Mob assignment and tick tests ────────────────────────────────── */

TEST(test_assign_and_remove)
{
    mc_mob_ai_init(mock_block_query);

    mc_error_t err = mc_mob_ai_assign(1, MOB_ZOMBIE);
    ASSERT(err == MC_OK);

    /* Duplicate assignment should fail */
    err = mc_mob_ai_assign(1, MOB_ZOMBIE);
    ASSERT(err == MC_ERR_ALREADY_EXISTS);

    /* Different entity is fine */
    err = mc_mob_ai_assign(2, MOB_SKELETON);
    ASSERT(err == MC_OK);

    /* Invalid mob type */
    err = mc_mob_ai_assign(3, MOB_TYPE_COUNT);
    ASSERT(err == MC_ERR_INVALID_ARG);

    /* Remove and re-assign */
    mc_mob_ai_remove(1);
    err = mc_mob_ai_assign(1, MOB_CREEPER);
    ASSERT(err == MC_OK);

    mc_mob_ai_shutdown();
}

TEST(test_tick_does_not_crash)
{
    mc_mob_ai_init(mock_block_query);

    mc_mob_ai_assign(10, MOB_ZOMBIE);
    mc_mob_ai_assign(11, MOB_SKELETON);
    mc_mob_ai_assign(12, MOB_CREEPER);
    mc_mob_ai_assign(13, MOB_PIG);
    mc_mob_ai_assign(14, MOB_COW);
    mc_mob_ai_assign(15, MOB_SHEEP);
    mc_mob_ai_assign(16, MOB_CHICKEN);

    /* Run several ticks — should not crash */
    for (tick_t t = 0; t < 100; t++) {
        mc_mob_ai_tick(t);
    }

    mc_mob_ai_shutdown();
}

TEST(test_init_null_query)
{
    mc_error_t err = mc_mob_ai_init(NULL);
    ASSERT(err == MC_ERR_INVALID_ARG);
}

/* ── Main ─────────────────────────────────────────────────────────── */

int main(void)
{
    printf("mc_mob_ai tests:\n");

    RUN(test_pathfind_flat_terrain);
    RUN(test_pathfind_unreachable);
    RUN(test_pathfind_same_position);
    RUN(test_assign_and_remove);
    RUN(test_tick_does_not_crash);
    RUN(test_init_null_query);

    printf("\n  %d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
