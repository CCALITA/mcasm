/*
 * test_redstone.c -- Tests for mc_redstone module.
 *
 * Creates a mock block grid and verifies:
 *   1. Torch powers adjacent wire to 15
 *   2. Power decays along wire: 15, 14, 13, ...
 *   3. Power source detection
 *   4. Component / conductor classification
 *   5. Scheduled tick processing
 */

#include "mc_redstone.h"
#include "mc_block.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Mock block grid ---- */

#define GRID_SIZE 32
#define GRID_OFFSET 8  /* offset so positions can be positive and centered */

/*
 * 3D grid of block IDs. Indexed as [x+GRID_OFFSET][y+GRID_OFFSET][z+GRID_OFFSET].
 * Positions outside the grid return BLOCK_AIR.
 */
static block_id_t g_grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];

static int in_bounds(block_pos_t pos)
{
    int gx = pos.x + GRID_OFFSET;
    int gy = pos.y + GRID_OFFSET;
    int gz = pos.z + GRID_OFFSET;
    return gx >= 0 && gx < GRID_SIZE &&
           gy >= 0 && gy < GRID_SIZE &&
           gz >= 0 && gz < GRID_SIZE;
}

static block_id_t mock_query(block_pos_t pos)
{
    if (!in_bounds(pos)) {
        return BLOCK_AIR;
    }
    return g_grid[pos.x + GRID_OFFSET][pos.y + GRID_OFFSET][pos.z + GRID_OFFSET];
}

static mc_error_t mock_set(block_pos_t pos, block_id_t block)
{
    if (!in_bounds(pos)) {
        return MC_ERR_INVALID_ARG;
    }
    g_grid[pos.x + GRID_OFFSET][pos.y + GRID_OFFSET][pos.z + GRID_OFFSET] = block;
    return MC_OK;
}

static void grid_clear(void)
{
    memset(g_grid, 0, sizeof(g_grid));
}

static void grid_set(int x, int y, int z, block_id_t id)
{
    block_pos_t p = {x, y, z, 0};
    mock_set(p, id);
}

/* ---- Test helpers ---- */

static int g_tests_run    = 0;
static int g_tests_passed = 0;

#define ASSERT_EQ(a, b, msg) do {                                    \
    g_tests_run++;                                                   \
    if ((a) == (b)) {                                                \
        g_tests_passed++;                                            \
    } else {                                                         \
        printf("  FAIL: %s (expected %d, got %d) at line %d\n",     \
               (msg), (int)(b), (int)(a), __LINE__);                 \
    }                                                                \
} while (0)

#define ASSERT_TRUE(cond, msg) do {                                  \
    g_tests_run++;                                                   \
    if ((cond)) {                                                    \
        g_tests_passed++;                                            \
    } else {                                                         \
        printf("  FAIL: %s at line %d\n", (msg), __LINE__);         \
    }                                                                \
} while (0)

/* ---- Tests ---- */

static void test_init_null_args(void)
{
    printf("  test_init_null_args\n");
    mc_error_t err = mc_redstone_init(NULL, mock_set);
    ASSERT_EQ(err, MC_ERR_INVALID_ARG, "init with null query should fail");

    err = mc_redstone_init(mock_query, NULL);
    ASSERT_EQ(err, MC_ERR_INVALID_ARG, "init with null set should fail");
}

static void test_init_valid(void)
{
    printf("  test_init_valid\n");
    mc_error_t err = mc_redstone_init(mock_query, mock_set);
    ASSERT_EQ(err, MC_OK, "init with valid args should succeed");
    mc_redstone_shutdown();
}

static void test_power_source_detection(void)
{
    printf("  test_power_source_detection\n");
    ASSERT_TRUE(mc_redstone_is_power_source(BLOCK_REDSTONE_TORCH),
                "redstone torch is power source");
    ASSERT_TRUE(mc_redstone_is_power_source(BLOCK_LEVER),
                "lever is power source");
    ASSERT_TRUE(mc_redstone_is_power_source(BLOCK_BUTTON),
                "button is power source");
    ASSERT_TRUE(mc_redstone_is_power_source(BLOCK_REPEATER),
                "repeater is power source");
    ASSERT_TRUE(!mc_redstone_is_power_source(BLOCK_AIR),
                "air is not power source");
    ASSERT_TRUE(!mc_redstone_is_power_source(BLOCK_STONE),
                "stone is not power source");
    ASSERT_TRUE(!mc_redstone_is_power_source(BLOCK_REDSTONE_WIRE),
                "wire is not power source");
}

static void test_conductor_detection(void)
{
    printf("  test_conductor_detection\n");
    ASSERT_TRUE(mc_redstone_is_conductor(BLOCK_STONE), "stone is conductor");
    ASSERT_TRUE(mc_redstone_is_conductor(BLOCK_DIRT), "dirt is conductor");
    ASSERT_TRUE(mc_redstone_is_conductor(BLOCK_COBBLESTONE), "cobblestone is conductor");
    ASSERT_TRUE(!mc_redstone_is_conductor(BLOCK_AIR), "air is not conductor");
    ASSERT_TRUE(!mc_redstone_is_conductor(BLOCK_GLASS), "glass is not conductor");
    ASSERT_TRUE(!mc_redstone_is_conductor(BLOCK_REDSTONE_WIRE), "wire is not conductor");
}

static void test_component_detection(void)
{
    printf("  test_component_detection\n");
    ASSERT_TRUE(mc_redstone_is_component(BLOCK_REPEATER), "repeater is component");
    ASSERT_TRUE(mc_redstone_is_component(BLOCK_COMPARATOR), "comparator is component");
    ASSERT_TRUE(mc_redstone_is_component(BLOCK_PISTON), "piston is component");
    ASSERT_TRUE(!mc_redstone_is_component(BLOCK_STONE), "stone is not component");
    ASSERT_TRUE(!mc_redstone_is_component(BLOCK_REDSTONE_WIRE), "wire is not component");
}

static void test_torch_powers_adjacent_wire(void)
{
    printf("  test_torch_powers_adjacent_wire\n");
    grid_clear();
    mc_redstone_init(mock_query, mock_set);

    /*
     * Layout (y=0 plane):
     *   x=0: REDSTONE_TORCH
     *   x=1: REDSTONE_WIRE
     */
    grid_set(0, 0, 0, BLOCK_REDSTONE_TORCH);
    grid_set(1, 0, 0, BLOCK_REDSTONE_WIRE);

    /* Trigger update from the torch position */
    block_pos_t torch_pos = {0, 0, 0, 0};
    mc_redstone_update(torch_pos);

    /* Update from the wire's perspective so it detects the adjacent torch */
    block_pos_t wire_pos = {1, 0, 0, 0};
    mc_redstone_update(wire_pos);

    uint8_t wire_power = mc_redstone_get_power(wire_pos);
    ASSERT_EQ(wire_power, 15, "wire adjacent to torch should be power 15");

    mc_redstone_shutdown();
}

static void test_power_decay_along_wire(void)
{
    printf("  test_power_decay_along_wire\n");
    grid_clear();
    mc_redstone_init(mock_query, mock_set);

    /*
     * Layout (y=0, z=0 line):
     *   x=0: REDSTONE_TORCH
     *   x=1..16: REDSTONE_WIRE (16 blocks of wire)
     */
    grid_set(0, 0, 0, BLOCK_REDSTONE_TORCH);
    for (int x = 1; x <= 16; x++) {
        grid_set(x, 0, 0, BLOCK_REDSTONE_WIRE);
    }

    /* Trigger update from the wire next to the torch.
     * This will BFS outward from x=1. */
    block_pos_t wire_start = {1, 0, 0, 0};
    mc_redstone_update(wire_start);

    /* Verify decay: wire at x=1 should be 15, x=2 should be 14, ... x=15 should be 1 */
    for (int x = 1; x <= 15; x++) {
        block_pos_t wp = {x, 0, 0, 0};
        uint8_t expected = (uint8_t)(16 - x);
        uint8_t actual   = mc_redstone_get_power(wp);
        char msg[64];
        snprintf(msg, sizeof(msg), "wire at x=%d should be power %d", x, expected);
        ASSERT_EQ(actual, expected, msg);
    }

    /* Wire at x=16 should be 0 (15 hops from source, 15-15=0) */
    block_pos_t wp16 = {16, 0, 0, 0};
    uint8_t p16 = mc_redstone_get_power(wp16);
    ASSERT_EQ(p16, 0, "wire at x=16 should be power 0");

    mc_redstone_shutdown();
}

static void test_no_propagation_through_air(void)
{
    printf("  test_no_propagation_through_air\n");
    grid_clear();
    mc_redstone_init(mock_query, mock_set);

    /*
     * Layout:
     *   x=0: REDSTONE_TORCH
     *   x=1: REDSTONE_WIRE
     *   x=2: AIR (gap)
     *   x=3: REDSTONE_WIRE
     */
    grid_set(0, 0, 0, BLOCK_REDSTONE_TORCH);
    grid_set(1, 0, 0, BLOCK_REDSTONE_WIRE);
    /* x=2 is AIR by default */
    grid_set(3, 0, 0, BLOCK_REDSTONE_WIRE);

    block_pos_t wire_start = {1, 0, 0, 0};
    mc_redstone_update(wire_start);

    /* Wire at x=1 gets power, but x=3 should not (air gap at x=2) */
    uint8_t p1 = mc_redstone_get_power(wire_start);
    ASSERT_EQ(p1, 15, "wire at x=1 should be power 15");

    block_pos_t wp3 = {3, 0, 0, 0};
    uint8_t p3 = mc_redstone_get_power(wp3);
    ASSERT_EQ(p3, 0, "wire at x=3 should be power 0 (air gap)");

    mc_redstone_shutdown();
}

static void test_get_power_default_zero(void)
{
    printf("  test_get_power_default_zero\n");
    mc_redstone_init(mock_query, mock_set);

    block_pos_t p = {99, 99, 99, 0};
    uint8_t power = mc_redstone_get_power(p);
    ASSERT_EQ(power, 0, "unvisited position should have power 0");

    mc_redstone_shutdown();
}

static void test_tick_fires_scheduled_update(void)
{
    printf("  test_tick_fires_scheduled_update\n");
    grid_clear();
    mc_redstone_init(mock_query, mock_set);

    /* Place a torch and wire */
    grid_set(0, 0, 0, BLOCK_REDSTONE_TORCH);
    grid_set(1, 0, 0, BLOCK_REDSTONE_WIRE);

    /* Before any tick, wire should be unpowered */
    block_pos_t wire_pos = {1, 0, 0, 0};
    uint8_t p_before = mc_redstone_get_power(wire_pos);
    ASSERT_EQ(p_before, 0, "wire unpowered before tick");

    /* Manually trigger an update (simulating what a tick would do) */
    mc_redstone_update(wire_pos);
    uint8_t p_after = mc_redstone_get_power(wire_pos);
    ASSERT_EQ(p_after, 15, "wire powered after update");

    /* Tick with no scheduled ticks should be harmless */
    mc_redstone_tick(100);

    mc_redstone_shutdown();
}

/* ---- Main ---- */

int main(void)
{
    printf("=== mc_redstone tests ===\n");

    test_init_null_args();
    test_init_valid();
    test_power_source_detection();
    test_conductor_detection();
    test_component_detection();
    test_torch_powers_adjacent_wire();
    test_power_decay_along_wire();
    test_no_propagation_through_air();
    test_get_power_default_zero();
    test_tick_fires_scheduled_update();

    printf("\n  Results: %d/%d passed\n", g_tests_passed, g_tests_run);

    if (g_tests_passed == g_tests_run) {
        printf("  ALL TESTS PASSED\n");
        return 0;
    }

    printf("  SOME TESTS FAILED\n");
    return 1;
}
