/*
 * test_timer.c -- unit tests for mc_platform timer functions.
 *
 * Timer tests do not require a GLFW window, so they are safe in CI.
 * We initialise GLFW just enough so glfwGetTime() works, then tear
 * it down at the end.
 */

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "mc_platform.h"
#include <stdio.h>

static int g_tests_run    = 0;
static int g_tests_passed = 0;

#define ASSERT(cond, msg)                                                \
    do {                                                                 \
        g_tests_run++;                                                   \
        if (!(cond)) {                                                   \
            fprintf(stderr, "  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        } else {                                                         \
            g_tests_passed++;                                            \
        }                                                                \
    } while (0)

static void test_get_time_monotonic(void) {
    double t1 = mc_platform_get_time();
    /* Burn a tiny amount of wall-clock time. */
    volatile int dummy = 0;
    for (int i = 0; i < 100000; i++) { dummy += i; }
    double t2 = mc_platform_get_time();

    ASSERT(t2 >= t1, "get_time should be monotonically non-decreasing");
}

static void test_delta_initial(void) {
    /* Before any update_timer call the delta should be the default 0.016. */
    float d = mc_platform_get_delta();
    ASSERT(d > 0.0f, "initial delta should be positive");
}

static void test_update_timer_produces_small_delta(void) {
    mc_platform_update_timer();
    /* Immediately call again -- delta should be very small. */
    mc_platform_update_timer();
    float d = mc_platform_get_delta();

    ASSERT(d >= 0.0f, "delta should be non-negative after update");
    ASSERT(d < 0.25f, "delta should be clamped below 0.25");
}

static void test_delta_clamped(void) {
    /* After update_timer the delta must be <= 0.25 (the clamp ceiling). */
    mc_platform_update_timer();
    float d = mc_platform_get_delta();
    ASSERT(d <= 0.25f, "delta should never exceed 0.25");
}

int main(void) {
    if (!glfwInit()) {
        fprintf(stderr, "glfwInit failed -- skipping timer tests\n");
        return 0; /* Do not fail CI if GLFW cannot init (e.g. headless). */
    }

    test_get_time_monotonic();
    test_delta_initial();
    test_update_timer_produces_small_delta();
    test_delta_clamped();

    glfwTerminate();

    printf("  timer: %d / %d passed\n", g_tests_passed, g_tests_run);
    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
