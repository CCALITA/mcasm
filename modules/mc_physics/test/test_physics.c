#include "mc_physics.h"
#include <stdio.h>
#include <math.h>

/* ---------- Mock block query: stone below y=64, air above ---------- */
static block_id_t mock_block_query(block_pos_t pos) {
    return (pos.y < 64) ? 1 : 0;   /* 1 = stone, 0 = air */
}

/* ---------- Test helpers ---------- */
static int g_tests_run   = 0;
static int g_tests_passed = 0;

#define ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL: %s (line %d)\n", msg, __LINE__); \
    } else { \
        g_tests_passed++; \
    } \
} while (0)

#define APPROX_EQ(a, b, eps) (fabsf((a) - (b)) < (eps))

/* ---------- Tests ---------- */

static void test_init(void) {
    mc_error_t err = mc_physics_init(mock_block_query);
    ASSERT(err == MC_OK, "physics init returns MC_OK");

    err = mc_physics_init(NULL);
    ASSERT(err == MC_ERR_INVALID_ARG, "physics init with NULL returns error");

    /* Re-init with valid fn for remaining tests */
    mc_physics_init(mock_block_query);
}

static void test_falling_stops_at_ground(void) {
    /* Player AABB at y=66, 0.6 wide, 1.8 tall, falling fast */
    aabb_t box = {
        .min = { 0.0f, 66.0f, 0.0f, 0.0f },
        .max = { 0.6f, 67.8f, 0.6f, 0.0f }
    };
    vec3_t velocity = { 0.0f, -20.0f, 0.0f, 0.0f };
    float dt = 1.0f;

    vec3_t disp = mc_physics_move_and_slide(box, velocity, dt);

    /* Should stop at y=64 (top of stone), so displacement should be -2.0 */
    ASSERT(APPROX_EQ(disp.y, -2.0f, 0.1f),
           "falling stops at y=64 ground");
    ASSERT(APPROX_EQ(disp.x, 0.0f, 0.001f),
           "no x displacement during fall");
    ASSERT(APPROX_EQ(disp.z, 0.0f, 0.001f),
           "no z displacement during fall");
}

static void test_horizontal_movement_in_air(void) {
    /* Player AABB above ground at y=65 */
    aabb_t box = {
        .min = { 10.0f, 65.0f, 10.0f, 0.0f },
        .max = { 10.6f, 66.8f, 10.6f, 0.0f }
    };
    vec3_t velocity = { 5.0f, 0.0f, 3.0f, 0.0f };
    float dt = 0.5f;

    vec3_t disp = mc_physics_move_and_slide(box, velocity, dt);

    ASSERT(APPROX_EQ(disp.x, 2.5f, 0.001f),
           "horizontal x movement succeeds in air");
    ASSERT(APPROX_EQ(disp.z, 1.5f, 0.001f),
           "horizontal z movement succeeds in air");
    ASSERT(APPROX_EQ(disp.y, 0.0f, 0.001f),
           "no y displacement for horizontal movement");
}

static void test_raycast_downward_hits_ground(void) {
    vec3_t origin    = { 5.5f, 70.0f, 5.5f, 0.0f };
    vec3_t direction = { 0.0f, -1.0f, 0.0f, 0.0f };

    raycast_result_t r = mc_physics_raycast(origin, direction, 100.0f);

    ASSERT(r.hit == 1, "raycast downward hits solid block");
    ASSERT(r.block_pos.y == 63, "raycast hits top block at y=63");
    ASSERT(APPROX_EQ(r.hit_normal.y, 1.0f, 0.001f),
           "hit normal points upward");
    ASSERT(r.distance > 0.0f && r.distance < 10.0f,
           "hit distance is reasonable");
}

static void test_raycast_upward_misses(void) {
    vec3_t origin    = { 5.5f, 65.0f, 5.5f, 0.0f };
    vec3_t direction = { 0.0f, 1.0f, 0.0f, 0.0f };

    raycast_result_t r = mc_physics_raycast(origin, direction, 50.0f);

    ASSERT(r.hit == 0, "raycast upward into air misses");
}

static void test_aabb_detects_solid(void) {
    /* Box overlapping stone below y=64 */
    aabb_t solid_box = {
        .min = { 3.0f, 62.0f, 3.0f, 0.0f },
        .max = { 4.0f, 64.0f, 4.0f, 0.0f }
    };
    ASSERT(mc_physics_test_aabb(solid_box) == 1,
           "test_aabb detects solid blocks");

    /* Box fully in air above y=64 */
    aabb_t air_box = {
        .min = { 3.0f, 65.0f, 3.0f, 0.0f },
        .max = { 4.0f, 66.0f, 4.0f, 0.0f }
    };
    ASSERT(mc_physics_test_aabb(air_box) == 0,
           "test_aabb returns 0 for air region");
}

static void test_fluid_flow_returns_zero(void) {
    block_pos_t pos = { 5, 64, 5, 0 };
    vec3_t flow = mc_physics_get_fluid_flow(pos);
    ASSERT(APPROX_EQ(flow.x, 0.0f, 0.001f) &&
           APPROX_EQ(flow.y, 0.0f, 0.001f) &&
           APPROX_EQ(flow.z, 0.0f, 0.001f),
           "fluid flow is zero (placeholder)");
}

static void test_shutdown(void) {
    mc_physics_shutdown();
    /* After shutdown, raycast should return no hit (null query) */
    vec3_t origin    = { 0.0f, 70.0f, 0.0f, 0.0f };
    vec3_t direction = { 0.0f, -1.0f, 0.0f, 0.0f };
    raycast_result_t r = mc_physics_raycast(origin, direction, 100.0f);
    ASSERT(r.hit == 0, "raycast returns no hit after shutdown");
}

int main(void) {
    printf("mc_physics tests:\n");

    test_init();
    test_falling_stops_at_ground();
    test_horizontal_movement_in_air();
    test_raycast_downward_hits_ground();
    test_raycast_upward_misses();
    test_aabb_detects_solid();
    test_fluid_flow_returns_zero();
    test_shutdown();

    printf("  %d/%d tests passed\n", g_tests_passed, g_tests_run);
    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
