/*
 * test_block.c -- Unit tests for the mc_block module.
 *
 * Tests cover:
 *   - mc_block_init registers all defaults correctly
 *   - mc_block_is_solid / mc_block_is_transparent / mc_block_get_light
 *   - mc_block_register with a custom block
 *   - Edge cases: out-of-range id, NULL props
 */

#include "mc_block.h"
#include <stdio.h>
#include <math.h>

static int tests_run    = 0;
static int tests_passed = 0;

#define ASSERT(cond, msg)                                               \
    do {                                                                \
        tests_run++;                                                    \
        if (!(cond)) {                                                  \
            fprintf(stderr, "  FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg); \
        } else {                                                        \
            tests_passed++;                                             \
        }                                                               \
    } while (0)

#define ASSERT_EQ_INT(a, b, msg) ASSERT((a) == (b), msg)
#define ASSERT_FLOAT_EQ(a, b, msg) ASSERT(fabsf((a) - (b)) < 0.001f, msg)

/*--- Test: init returns MC_OK ---*/
static void test_init(void)
{
    mc_error_t err = mc_block_init();
    ASSERT_EQ_INT(err, MC_OK, "mc_block_init returns MC_OK");
}

/*--- Test: AIR properties ---*/
static void test_air(void)
{
    const block_properties_t* p = mc_block_get_properties(BLOCK_AIR);
    ASSERT(p != NULL, "AIR properties not NULL");
    ASSERT_EQ_INT(p->transparent, 1, "AIR is transparent");
    ASSERT_EQ_INT(p->solid, 0, "AIR is not solid");
    ASSERT_EQ_INT(p->light_emission, 0, "AIR emits no light");
    ASSERT_FLOAT_EQ(p->hardness, 0.0f, "AIR hardness is 0");

    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_AIR), 0, "is_solid(AIR) == 0");
    ASSERT_EQ_INT(mc_block_is_transparent(BLOCK_AIR), 1, "is_transparent(AIR) == 1");
    ASSERT_EQ_INT(mc_block_get_light(BLOCK_AIR), 0, "get_light(AIR) == 0");
}

/*--- Test: STONE properties ---*/
static void test_stone(void)
{
    const block_properties_t* p = mc_block_get_properties(BLOCK_STONE);
    ASSERT(p != NULL, "STONE properties not NULL");
    ASSERT_EQ_INT(p->solid, 1, "STONE is solid");
    ASSERT_EQ_INT(p->transparent, 0, "STONE is not transparent");
    ASSERT_FLOAT_EQ(p->hardness, 1.5f, "STONE hardness is 1.5");

    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_STONE), 1, "is_solid(STONE) == 1");
    ASSERT_EQ_INT(mc_block_is_transparent(BLOCK_STONE), 0, "is_transparent(STONE) == 0");
}

/*--- Test: BEDROCK hardness = -1 ---*/
static void test_bedrock(void)
{
    const block_properties_t* p = mc_block_get_properties(BLOCK_BEDROCK);
    ASSERT(p != NULL, "BEDROCK properties not NULL");
    ASSERT_FLOAT_EQ(p->hardness, -1.0f, "BEDROCK hardness is -1 (indestructible)");
    ASSERT_EQ_INT(p->solid, 1, "BEDROCK is solid");
}

/*--- Test: WATER properties ---*/
static void test_water(void)
{
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_WATER), 0, "WATER is not solid");
    ASSERT_EQ_INT(mc_block_is_transparent(BLOCK_WATER), 1, "WATER is transparent");
    ASSERT_EQ_INT(mc_block_get_light(BLOCK_WATER), 0, "WATER emits no light");
}

/*--- Test: LAVA light emission ---*/
static void test_lava(void)
{
    ASSERT_EQ_INT(mc_block_get_light(BLOCK_LAVA), 15, "LAVA light == 15");
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_LAVA), 0, "LAVA is not solid");
}

/*--- Test: TORCH light emission ---*/
static void test_torch(void)
{
    ASSERT_EQ_INT(mc_block_get_light(BLOCK_TORCH), 14, "TORCH light == 14");
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_TORCH), 0, "TORCH is not solid");
    ASSERT_EQ_INT(mc_block_is_transparent(BLOCK_TORCH), 1, "TORCH is transparent");
}

/*--- Test: REDSTONE_TORCH light emission ---*/
static void test_redstone_torch(void)
{
    ASSERT_EQ_INT(mc_block_get_light(BLOCK_REDSTONE_TORCH), 7,
                  "REDSTONE_TORCH light == 7");
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_REDSTONE_TORCH), 0,
                  "REDSTONE_TORCH is not solid");
}

/*--- Test: GLASS and OAK_LEAVES transparent ---*/
static void test_transparent_blocks(void)
{
    ASSERT_EQ_INT(mc_block_is_transparent(BLOCK_GLASS), 1, "GLASS is transparent");
    ASSERT_EQ_INT(mc_block_is_transparent(BLOCK_OAK_LEAVES), 1,
                  "OAK_LEAVES is transparent");
}

/*--- Test: Solid ores ---*/
static void test_ores(void)
{
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_IRON_ORE), 1, "IRON_ORE is solid");
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_COAL_ORE), 1, "COAL_ORE is solid");
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_GOLD_ORE), 1, "GOLD_ORE is solid");
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_DIAMOND_ORE), 1, "DIAMOND_ORE is solid");
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_REDSTONE_ORE), 1, "REDSTONE_ORE is solid");
    ASSERT_EQ_INT(mc_block_is_transparent(BLOCK_IRON_ORE), 0,
                  "IRON_ORE is not transparent");
}

/*--- Test: Register a custom block ---*/
static void test_register_custom(void)
{
    block_properties_t custom = {
        .hardness       = 5.0f,
        .transparent    = 0,
        .solid          = 1,
        .light_emission = 10,
        .flammable      = 1,
        .texture_top    = 100,
        .texture_side   = 101,
        .texture_bottom = 102,
    };

    mc_error_t err = mc_block_register(200, &custom);
    ASSERT_EQ_INT(err, MC_OK, "register custom block (id=200) succeeds");

    const block_properties_t* p = mc_block_get_properties(200);
    ASSERT(p != NULL, "custom block properties not NULL");
    ASSERT_FLOAT_EQ(p->hardness, 5.0f, "custom hardness == 5.0");
    ASSERT_EQ_INT(p->solid, 1, "custom is solid");
    ASSERT_EQ_INT(p->light_emission, 10, "custom light == 10");
    ASSERT_EQ_INT(p->flammable, 1, "custom is flammable");
    ASSERT_EQ_INT(p->texture_top, 100, "custom texture_top == 100");
    ASSERT_EQ_INT(p->texture_side, 101, "custom texture_side == 101");
    ASSERT_EQ_INT(p->texture_bottom, 102, "custom texture_bottom == 102");

    ASSERT_EQ_INT(mc_block_is_solid(200), 1, "is_solid(custom) == 1");
    ASSERT_EQ_INT(mc_block_get_light(200), 10, "get_light(custom) == 10");
}

/*--- Test: Out-of-range id handling ---*/
static void test_out_of_range(void)
{
    mc_error_t err = mc_block_register(BLOCK_TYPE_COUNT, NULL);
    ASSERT_EQ_INT(err, MC_ERR_INVALID_ARG,
                  "register with id >= BLOCK_TYPE_COUNT fails");

    const block_properties_t* p = mc_block_get_properties(BLOCK_TYPE_COUNT);
    ASSERT(p == NULL, "get_properties with out-of-range id returns NULL");

    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_TYPE_COUNT), 0,
                  "is_solid with out-of-range id returns 0");
    ASSERT_EQ_INT(mc_block_is_transparent(BLOCK_TYPE_COUNT), 1,
                  "is_transparent with out-of-range id returns 1");
    ASSERT_EQ_INT(mc_block_get_light(BLOCK_TYPE_COUNT), 0,
                  "get_light with out-of-range id returns 0");
}

/*--- Test: NULL props pointer rejected ---*/
static void test_null_props(void)
{
    mc_error_t err = mc_block_register(100, NULL);
    ASSERT_EQ_INT(err, MC_ERR_INVALID_ARG,
                  "register with NULL props returns MC_ERR_INVALID_ARG");
}

/*--- Test: Redstone components are not solid ---*/
static void test_redstone_components(void)
{
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_REDSTONE_WIRE), 0,
                  "REDSTONE_WIRE is not solid");
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_LEVER), 0, "LEVER is not solid");
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_BUTTON), 0, "BUTTON is not solid");
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_REPEATER), 0, "REPEATER is not solid");
    ASSERT_EQ_INT(mc_block_is_solid(BLOCK_COMPARATOR), 0,
                  "COMPARATOR is not solid");
}

/*--- Test: GRASS texture mapping ---*/
static void test_grass_textures(void)
{
    const block_properties_t* p = mc_block_get_properties(BLOCK_GRASS);
    ASSERT(p != NULL, "GRASS properties not NULL");
    ASSERT_EQ_INT(p->texture_top, 1, "GRASS texture_top == 1");
    ASSERT_EQ_INT(p->texture_side, 2, "GRASS texture_side == 2");
    ASSERT_EQ_INT(p->texture_bottom, 3, "GRASS texture_bottom == 3");
}

int main(void)
{
    test_init();
    test_air();
    test_stone();
    test_bedrock();
    test_water();
    test_lava();
    test_torch();
    test_redstone_torch();
    test_transparent_blocks();
    test_ores();
    test_register_custom();
    test_out_of_range();
    test_null_props();
    test_redstone_components();
    test_grass_textures();

    printf("  %d/%d tests passed\n", tests_passed, tests_run);

    mc_block_shutdown();

    return (tests_passed == tests_run) ? 0 : 1;
}
