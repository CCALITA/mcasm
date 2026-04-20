#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mc_crafting.h"
#include "mc_block.h"

/* Item IDs mirrored from internal header for test assertions. */
#define ITEM_STICK             256
#define ITEM_COAL              257
#define ITEM_IRON_INGOT        258
#define ITEM_WOODEN_PICKAXE    261
#define ITEM_STONE_PICKAXE     262
#define ITEM_IRON_PICKAXE      263

static int tests_run    = 0;
static int tests_passed = 0;

#define ASSERT(cond, msg)                                      \
    do {                                                       \
        tests_run++;                                           \
        if (!(cond)) {                                         \
            fprintf(stderr, "FAIL [%s:%d] %s\n",              \
                    __FILE__, __LINE__, (msg));                 \
            return;                                            \
        }                                                      \
        tests_passed++;                                        \
    } while (0)

#define ASSERT_EQ(a, b, msg) ASSERT((a) == (b), (msg))

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

static void grid_clear(block_id_t grid[9]) {
    memset(grid, 0, sizeof(block_id_t) * 9);
}

/* ------------------------------------------------------------------ */
/*  Tests: recipe registration and count                               */
/* ------------------------------------------------------------------ */

static void test_init_loads_defaults(void) {
    mc_crafting_init();
    uint32_t count = mc_crafting_recipe_count();
    ASSERT(count > 0, "defaults should register at least one recipe");
    mc_crafting_shutdown();
}

static void test_register_custom(void) {
    mc_crafting_init();
    uint32_t before = mc_crafting_recipe_count();
    mc_recipe_t r = {
        .grid         = { BLOCK_DIRT, 0 },
        .width        = 1,
        .height       = 1,
        .result       = BLOCK_GRASS,
        .result_count = 1,
        .shapeless    = 0
    };
    mc_error_t err = mc_crafting_register(&r);
    ASSERT_EQ(err, MC_OK, "register should succeed");
    ASSERT_EQ(mc_crafting_recipe_count(), before + 1,
              "count should increase by 1");
    mc_crafting_shutdown();
}

static void test_register_null(void) {
    mc_crafting_init();
    mc_error_t err = mc_crafting_register(NULL);
    ASSERT_EQ(err, MC_ERR_INVALID_ARG, "register(NULL) should fail");
    mc_crafting_shutdown();
}

/* ------------------------------------------------------------------ */
/*  Tests: shaped matching                                             */
/* ------------------------------------------------------------------ */

static void test_match_planks_from_log(void) {
    mc_crafting_init();
    block_id_t grid[9];
    grid_clear(grid);
    /* Planks from log is shapeless — place log anywhere. */
    grid[4] = BLOCK_OAK_LOG;
    uint8_t count = 0;
    block_id_t result = mc_crafting_match(grid, &count);
    ASSERT_EQ(result, BLOCK_OAK_PLANKS, "log should craft planks");
    ASSERT_EQ(count, 4, "planks yield should be 4");
    mc_crafting_shutdown();
}

static void test_match_crafting_table(void) {
    mc_crafting_init();
    block_id_t grid[9];
    grid_clear(grid);
    /* 2x2 planks in top-left. */
    grid[0] = BLOCK_OAK_PLANKS; grid[1] = BLOCK_OAK_PLANKS;
    grid[3] = BLOCK_OAK_PLANKS; grid[4] = BLOCK_OAK_PLANKS;
    uint8_t count = 0;
    block_id_t result = mc_crafting_match(grid, &count);
    ASSERT_EQ(result, BLOCK_CRAFTING_TABLE, "4 planks should craft table");
    ASSERT_EQ(count, 1, "table count should be 1");
    mc_crafting_shutdown();
}

static void test_match_sticks(void) {
    mc_crafting_init();
    block_id_t grid[9];
    grid_clear(grid);
    /* Vertical pair of planks in center column. */
    grid[1] = BLOCK_OAK_PLANKS;
    grid[4] = BLOCK_OAK_PLANKS;
    uint8_t count = 0;
    block_id_t result = mc_crafting_match(grid, &count);
    ASSERT_EQ(result, ITEM_STICK, "2 vertical planks should craft sticks");
    ASSERT_EQ(count, 4, "sticks yield should be 4");
    mc_crafting_shutdown();
}

static void test_match_furnace(void) {
    mc_crafting_init();
    block_id_t grid[9];
    /* 3x3 ring of cobblestone. */
    for (int i = 0; i < 9; i++) {
        grid[i] = BLOCK_COBBLESTONE;
    }
    grid[4] = BLOCK_AIR; /* center empty */
    uint8_t count = 0;
    block_id_t result = mc_crafting_match(grid, &count);
    ASSERT_EQ(result, BLOCK_FURNACE, "8 cobblestone ring should craft furnace");
    ASSERT_EQ(count, 1, "furnace count should be 1");
    mc_crafting_shutdown();
}

static void test_match_wooden_pickaxe(void) {
    mc_crafting_init();
    block_id_t grid[9];
    grid_clear(grid);
    grid[0] = BLOCK_OAK_PLANKS; grid[1] = BLOCK_OAK_PLANKS; grid[2] = BLOCK_OAK_PLANKS;
    grid[4] = ITEM_STICK;
    grid[7] = ITEM_STICK;
    uint8_t count = 0;
    block_id_t result = mc_crafting_match(grid, &count);
    ASSERT_EQ(result, ITEM_WOODEN_PICKAXE, "should craft wooden pickaxe");
    ASSERT_EQ(count, 1, "pickaxe count should be 1");
    mc_crafting_shutdown();
}

static void test_match_no_match(void) {
    mc_crafting_init();
    block_id_t grid[9];
    grid_clear(grid);
    grid[0] = BLOCK_DIRT;
    grid[1] = BLOCK_SAND;
    grid[2] = BLOCK_GRAVEL;
    uint8_t count = 99;
    block_id_t result = mc_crafting_match(grid, &count);
    ASSERT_EQ(result, BLOCK_AIR, "random items should not match");
    ASSERT_EQ(count, 0, "count should be 0 on no match");
    mc_crafting_shutdown();
}

/* ------------------------------------------------------------------ */
/*  Tests: shapeless matching                                          */
/* ------------------------------------------------------------------ */

static void test_shapeless_any_position(void) {
    mc_crafting_init();
    /* Log at various grid positions should all yield planks. */
    for (int pos = 0; pos < 9; pos++) {
        block_id_t grid[9];
        grid_clear(grid);
        grid[pos] = BLOCK_OAK_LOG;
        uint8_t count = 0;
        block_id_t result = mc_crafting_match(grid, &count);
        ASSERT_EQ(result, BLOCK_OAK_PLANKS,
                  "shapeless log should match at any position");
        ASSERT_EQ(count, 4, "planks yield should be 4");
    }
    mc_crafting_shutdown();
}

static void test_shapeless_torch(void) {
    mc_crafting_init();
    block_id_t grid[9];
    /* Coal and stick in arbitrary slots. */
    grid_clear(grid);
    grid[2] = ITEM_STICK;
    grid[6] = ITEM_COAL;
    uint8_t count = 0;
    block_id_t result = mc_crafting_match(grid, &count);
    ASSERT_EQ(result, BLOCK_TORCH, "stick + coal should craft torch");
    ASSERT_EQ(count, 4, "torch yield should be 4");
    mc_crafting_shutdown();
}

/* ------------------------------------------------------------------ */
/*  Tests: shaped recipe at shifted position                           */
/* ------------------------------------------------------------------ */

static void test_shaped_offset(void) {
    mc_crafting_init();
    block_id_t grid[9];
    grid_clear(grid);
    /* 2x2 planks in bottom-right corner. */
    grid[4] = BLOCK_OAK_PLANKS; grid[5] = BLOCK_OAK_PLANKS;
    grid[7] = BLOCK_OAK_PLANKS; grid[8] = BLOCK_OAK_PLANKS;
    uint8_t count = 0;
    block_id_t result = mc_crafting_match(grid, &count);
    ASSERT_EQ(result, BLOCK_CRAFTING_TABLE,
              "2x2 planks offset to bottom-right should match");
    mc_crafting_shutdown();
}

/* ------------------------------------------------------------------ */
/*  Tests: smelting                                                    */
/* ------------------------------------------------------------------ */

static void test_smelt_iron_ore(void) {
    mc_crafting_init();
    float duration = 0.0f;
    block_id_t result = mc_crafting_smelt(BLOCK_IRON_ORE, &duration);
    ASSERT_EQ(result, ITEM_IRON_INGOT, "iron ore should smelt to ingot");
    ASSERT(fabsf(duration - 10.0f) < 0.01f, "iron smelt duration should be 10");
    mc_crafting_shutdown();
}

static void test_smelt_sand(void) {
    mc_crafting_init();
    float duration = 0.0f;
    block_id_t result = mc_crafting_smelt(BLOCK_SAND, &duration);
    ASSERT_EQ(result, BLOCK_GLASS, "sand should smelt to glass");
    mc_crafting_shutdown();
}

static void test_smelt_cobblestone(void) {
    mc_crafting_init();
    float duration = 0.0f;
    block_id_t result = mc_crafting_smelt(BLOCK_COBBLESTONE, &duration);
    ASSERT_EQ(result, BLOCK_STONE, "cobblestone should smelt to stone");
    mc_crafting_shutdown();
}

static void test_smelt_unknown(void) {
    mc_crafting_init();
    float duration = 99.0f;
    block_id_t result = mc_crafting_smelt(BLOCK_DIRT, &duration);
    ASSERT_EQ(result, BLOCK_AIR, "unknown item should not smelt");
    ASSERT(fabsf(duration) < 0.01f, "duration should be 0 on no match");
    mc_crafting_shutdown();
}

/* ------------------------------------------------------------------ */
/*  Tests: fuel values                                                 */
/* ------------------------------------------------------------------ */

static void test_fuel_coal(void) {
    mc_crafting_init();
    float val = mc_crafting_fuel_value(ITEM_COAL);
    ASSERT(fabsf(val - 80.0f) < 0.01f, "coal fuel value should be 80");
    mc_crafting_shutdown();
}

static void test_fuel_log(void) {
    mc_crafting_init();
    float val = mc_crafting_fuel_value(BLOCK_OAK_LOG);
    ASSERT(fabsf(val - 15.0f) < 0.01f, "log fuel value should be 15");
    mc_crafting_shutdown();
}

static void test_fuel_stick(void) {
    mc_crafting_init();
    float val = mc_crafting_fuel_value(ITEM_STICK);
    ASSERT(fabsf(val - 5.0f) < 0.01f, "stick fuel value should be 5");
    mc_crafting_shutdown();
}

static void test_fuel_unknown(void) {
    mc_crafting_init();
    float val = mc_crafting_fuel_value(BLOCK_DIRT);
    ASSERT(fabsf(val) < 0.01f, "dirt should have 0 fuel value");
    mc_crafting_shutdown();
}

/* ------------------------------------------------------------------ */
/*  Test: get_recipe                                                   */
/* ------------------------------------------------------------------ */

static void test_get_recipe(void) {
    mc_crafting_init();
    const mc_recipe_t *r = mc_crafting_get_recipe(0);
    ASSERT(r != NULL, "first recipe should exist");
    ASSERT(r->result != BLOCK_AIR, "first recipe result should not be AIR");

    const mc_recipe_t *bad = mc_crafting_get_recipe(9999);
    ASSERT(bad == NULL, "out-of-bounds index should return NULL");
    mc_crafting_shutdown();
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int main(void) {
    /* Registration */
    test_init_loads_defaults();
    test_register_custom();
    test_register_null();

    /* Shaped matching */
    test_match_planks_from_log();
    test_match_crafting_table();
    test_match_sticks();
    test_match_furnace();
    test_match_wooden_pickaxe();
    test_match_no_match();

    /* Shapeless matching */
    test_shapeless_any_position();
    test_shapeless_torch();

    /* Shaped offset */
    test_shaped_offset();

    /* Smelting */
    test_smelt_iron_ore();
    test_smelt_sand();
    test_smelt_cobblestone();
    test_smelt_unknown();

    /* Fuel values */
    test_fuel_coal();
    test_fuel_log();
    test_fuel_stick();
    test_fuel_unknown();

    /* get_recipe */
    test_get_recipe();

    printf("%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
