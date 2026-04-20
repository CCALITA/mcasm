#include "mc_particle.h"

#include <stdio.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/*  Minimal test harness                                               */
/* ------------------------------------------------------------------ */

static int g_tests_run;
static int g_tests_passed;

#define ASSERT(cond, msg)                                               \
    do {                                                                \
        g_tests_run++;                                                  \
        if (!(cond)) {                                                  \
            fprintf(stderr, "  FAIL [%s:%d] %s\n", __FILE__,           \
                    __LINE__, (msg));                                    \
        } else {                                                        \
            g_tests_passed++;                                           \
        }                                                               \
    } while (0)

#define ASSERT_EQ_U32(a, b, msg)  ASSERT((a) == (b), (msg))
#define ASSERT_NEAR(a, b, eps, msg) ASSERT(fabsf((a) - (b)) < (eps), (msg))

/*
 * Access the module-internal pool for white-box position checks.
 * This symbol is not static — it is defined in particle.c.
 */
#define MAX_PARTICLES 4096

typedef struct {
    float pos_x[MAX_PARTICLES];
    float pos_y[MAX_PARTICLES];
    float pos_z[MAX_PARTICLES];
    float vel_x[MAX_PARTICLES];
    float vel_y[MAX_PARTICLES];
    float vel_z[MAX_PARTICLES];
    float lifetime[MAX_PARTICLES];
    float remaining_life[MAX_PARTICLES];
    uint8_t type[MAX_PARTICLES];
    float color_r[MAX_PARTICLES];
    float color_g[MAX_PARTICLES];
    float color_b[MAX_PARTICLES];
    float color_a[MAX_PARTICLES];
    uint32_t count;
} particle_pool_t;

extern particle_pool_t g_pool;

/* ------------------------------------------------------------------ */
/*  Tests                                                              */
/* ------------------------------------------------------------------ */

static void test_init_shutdown(void)
{
    mc_error_t err = mc_particle_init();
    ASSERT(err == MC_OK, "init returns MC_OK");
    ASSERT_EQ_U32(mc_particle_count(), 0, "count is 0 after init");
    mc_particle_shutdown();
    ASSERT_EQ_U32(mc_particle_count(), 0, "count is 0 after shutdown");
}

static void test_emit_basic(void)
{
    mc_particle_init();

    vec3_t pos = { 1.0f, 2.0f, 3.0f, 0.0f };
    vec3_t vel = { 0.0f, 5.0f, 0.0f, 0.0f };

    mc_particle_emit(PARTICLE_FLAME, pos, vel, 1.0f, 10);
    ASSERT_EQ_U32(mc_particle_count(), 10, "10 particles after emit");

    mc_particle_emit(PARTICLE_SMOKE, pos, vel, 2.0f, 5);
    ASSERT_EQ_U32(mc_particle_count(), 15, "15 particles after second emit");

    mc_particle_shutdown();
}

static void test_tick_lifetime_expiry(void)
{
    mc_particle_init();

    vec3_t pos = { 0.0f, 0.0f, 0.0f, 0.0f };
    vec3_t vel = { 0.0f, 0.0f, 0.0f, 0.0f };

    /* Emit 8 particles with 1.0s lifetime */
    mc_particle_emit(PARTICLE_DUST, pos, vel, 1.0f, 8);
    ASSERT_EQ_U32(mc_particle_count(), 8, "8 particles alive");

    /* Tick 0.5s — all should still be alive */
    mc_particle_tick(0.5f);
    ASSERT_EQ_U32(mc_particle_count(), 8, "8 alive after 0.5s");

    /* Tick another 0.6s — all should be dead (total 1.1s > 1.0s) */
    mc_particle_tick(0.6f);
    ASSERT_EQ_U32(mc_particle_count(), 0, "0 alive after 1.1s total");

    mc_particle_shutdown();
}

static void test_tick_mixed_lifetimes(void)
{
    mc_particle_init();

    vec3_t pos = { 0.0f, 0.0f, 0.0f, 0.0f };
    vec3_t vel = { 0.0f, 0.0f, 0.0f, 0.0f };

    /* 4 short-lived (0.5s) + 4 long-lived (2.0s) */
    mc_particle_emit(PARTICLE_FLAME, pos, vel, 0.5f, 4);
    mc_particle_emit(PARTICLE_RAIN,  pos, vel, 2.0f, 4);
    ASSERT_EQ_U32(mc_particle_count(), 8, "8 total");

    /* Tick 0.6s — short-lived should be dead */
    mc_particle_tick(0.6f);
    ASSERT_EQ_U32(mc_particle_count(), 4, "4 long-lived remain");

    /* Tick 1.5s more — all dead */
    mc_particle_tick(1.5f);
    ASSERT_EQ_U32(mc_particle_count(), 0, "all expired");

    mc_particle_shutdown();
}

static void test_position_update(void)
{
    mc_particle_init();

    vec3_t pos = { 10.0f, 20.0f, 30.0f, 0.0f };
    vec3_t vel = { 1.0f,  0.0f,  -1.0f, 0.0f };

    mc_particle_emit(PARTICLE_DUST, pos, vel, 5.0f, 1);

    /*
     * After dt=1.0 the ASM/fallback applies:
     *   vel_y  = 0 - 9.81*1 = -9.81
     *   pos_x  = 10 + 1*1   = 11
     *   pos_y  = 20 + (-9.81)*1 = 10.19
     *   pos_z  = 30 + (-1)*1 = 29
     *
     * Then tick decrements remaining_life by 1.0 (4.0 left).
     */
    mc_particle_tick(1.0f);

    ASSERT_EQ_U32(mc_particle_count(), 1, "particle alive after 1s tick");

    ASSERT_NEAR(g_pool.pos_x[0], 11.0f, 0.01f, "pos_x updated");
    ASSERT_NEAR(g_pool.pos_y[0], 10.19f, 0.01f, "pos_y updated with gravity");
    ASSERT_NEAR(g_pool.pos_z[0], 29.0f, 0.01f, "pos_z updated");
    ASSERT_NEAR(g_pool.vel_y[0], -9.81f, 0.01f, "vel_y after gravity");

    mc_particle_shutdown();
}

static void test_pool_full(void)
{
    mc_particle_init();

    vec3_t pos = { 0.0f, 0.0f, 0.0f, 0.0f };
    vec3_t vel = { 0.0f, 0.0f, 0.0f, 0.0f };

    /* Fill the pool to capacity (4096) */
    mc_particle_emit(PARTICLE_BUBBLE, pos, vel, 10.0f, 4096);
    ASSERT_EQ_U32(mc_particle_count(), 4096, "pool at capacity");

    /* Attempt to emit more — should gracefully cap at max */
    mc_particle_emit(PARTICLE_BUBBLE, pos, vel, 10.0f, 100);
    ASSERT_EQ_U32(mc_particle_count(), 4096, "still 4096 when full");

    mc_particle_shutdown();
}

static void test_emit_zero_count(void)
{
    mc_particle_init();

    vec3_t pos = { 0.0f, 0.0f, 0.0f, 0.0f };
    vec3_t vel = { 0.0f, 0.0f, 0.0f, 0.0f };

    mc_particle_emit(PARTICLE_FLAME, pos, vel, 1.0f, 0);
    ASSERT_EQ_U32(mc_particle_count(), 0, "no particles from zero count");

    mc_particle_shutdown();
}

static void test_tick_zero_dt(void)
{
    mc_particle_init();

    vec3_t pos = { 1.0f, 2.0f, 3.0f, 0.0f };
    vec3_t vel = { 1.0f, 1.0f, 1.0f, 0.0f };

    mc_particle_emit(PARTICLE_SMOKE, pos, vel, 1.0f, 4);

    /* dt=0 should be a no-op */
    mc_particle_tick(0.0f);
    ASSERT_EQ_U32(mc_particle_count(), 4, "unchanged with dt=0");

    mc_particle_shutdown();
}

static void test_multiple_emit_tick_cycles(void)
{
    mc_particle_init();

    vec3_t pos = { 0.0f, 0.0f, 0.0f, 0.0f };
    vec3_t vel = { 0.0f, 1.0f, 0.0f, 0.0f };

    /* Cycle 1: emit 4, tick half their lifetime */
    mc_particle_emit(PARTICLE_EXPLOSION, pos, vel, 1.0f, 4);
    mc_particle_tick(0.3f);
    ASSERT_EQ_U32(mc_particle_count(), 4, "cycle1: 4 alive");

    /* Cycle 2: emit 4 more, tick to kill first batch */
    mc_particle_emit(PARTICLE_SPLASH, pos, vel, 2.0f, 4);
    ASSERT_EQ_U32(mc_particle_count(), 8, "cycle2: 8 total");

    mc_particle_tick(0.8f);
    /* First batch: remaining = 1.0 - 0.3 - 0.8 = -0.1 -> dead */
    /* Second batch: remaining = 2.0 - 0.8 = 1.2 -> alive */
    ASSERT_EQ_U32(mc_particle_count(), 4, "cycle2: first batch dead");

    mc_particle_shutdown();
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int main(void)
{
    printf("mc_particle tests\n");

    test_init_shutdown();
    test_emit_basic();
    test_tick_lifetime_expiry();
    test_tick_mixed_lifetimes();
    test_position_update();
    test_pool_full();
    test_emit_zero_count();
    test_tick_zero_dt();
    test_multiple_emit_tick_cycles();

    printf("  %d / %d passed\n", g_tests_passed, g_tests_run);

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
