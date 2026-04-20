/*
 * test_input.c — Unit tests for mc_input module.
 *
 * We mock mc_platform functions so we can run without a real window.
 * The test manipulates g_input directly for state-transition tests.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mc_input.h"

/* ---- pull in internal state for direct manipulation in tests ---- */
#include "../internal/input_internal.h"

/* ================================================================
 *  Mock platform layer
 * ================================================================ */
static uint8_t mock_keys[KEY_MAX];
static float   mock_mouse_dx, mock_mouse_dy;

uint8_t mc_platform_key_held(int key)
{
    if (key < 0 || key >= KEY_MAX) { return 0; }
    return mock_keys[key];
}

uint8_t mc_platform_key_pressed(int key)
{
    (void)key;
    return 0;
}

void mc_platform_get_mouse_delta(float* dx, float* dy)
{
    if (dx) { *dx = mock_mouse_dx; }
    if (dy) { *dy = mock_mouse_dy; }
}

static void mock_reset(void)
{
    memset(mock_keys, 0, sizeof(mock_keys));
    mock_mouse_dx = 0.0f;
    mock_mouse_dy = 0.0f;
}

/* ================================================================
 *  Minimal test framework
 * ================================================================ */
static int tests_run    = 0;
static int tests_passed = 0;

#define ASSERT(cond, msg) do {                                              \
    tests_run++;                                                            \
    if (!(cond)) {                                                          \
        printf("  FAIL: %s (line %d): %s\n", __func__, __LINE__, (msg));    \
        return;                                                             \
    }                                                                       \
    tests_passed++;                                                         \
} while (0)

#define ASSERT_EQ_INT(a, b, msg) ASSERT((a) == (b), (msg))
#define ASSERT_FLOAT_EQ(a, b, msg) ASSERT(fabsf((a) - (b)) < 1e-6f, (msg))

/* ================================================================
 *  Tests: default bindings
 * ================================================================ */
static void test_default_bindings(void)
{
    mc_input_init();
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_FORWARD),   KEY_W,            "forward = W");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_BACKWARD),  KEY_S,            "backward = S");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_LEFT),      KEY_A,            "left = A");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_RIGHT),     KEY_D,            "right = D");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_JUMP),      KEY_SPACE,        "jump = Space");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_SNEAK),     KEY_LEFT_SHIFT,   "sneak = LShift");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_SPRINT),    KEY_LEFT_CONTROL, "sprint = LCtrl");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_INVENTORY), KEY_E,            "inventory = E");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_CHAT),      KEY_T,            "chat = T");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_PAUSE),     KEY_ESCAPE,       "pause = Escape");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_DEBUG),     KEY_F3,           "debug = F3");

    /* Hotbar 1-9 */
    for (int i = 0; i < 9; i++) {
        ASSERT_EQ_INT(mc_input_get_binding(ACTION_HOTBAR_1 + i), KEY_1 + i, "hotbar slot");
    }
    mc_input_shutdown();
}

/* ================================================================
 *  Tests: rebinding
 * ================================================================ */
static void test_rebind_key(void)
{
    mc_input_init();
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_FORWARD), KEY_W, "default forward");

    mc_error_t err = mc_input_bind_key(ACTION_FORWARD, KEY_E);
    ASSERT_EQ_INT(err, MC_OK, "rebind returns OK");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_FORWARD), KEY_E, "forward rebound to E");

    /* Invalid action */
    err = mc_input_bind_key(ACTION_COUNT, KEY_W);
    ASSERT_EQ_INT(err, MC_ERR_INVALID_ARG, "out-of-range action rejected");

    /* Invalid key */
    err = mc_input_bind_key(ACTION_FORWARD, -1);
    ASSERT_EQ_INT(err, MC_ERR_INVALID_ARG, "negative key rejected");

    mc_input_shutdown();
}

/* ================================================================
 *  Tests: action state transitions (pressed / held / released)
 * ================================================================ */
static void test_action_pressed(void)
{
    mc_input_init();
    mock_reset();

    /* Frame 1: key not down */
    mc_input_update();
    ASSERT_EQ_INT(mc_input_action_pressed(ACTION_FORWARD), 0, "not pressed when key up");
    ASSERT_EQ_INT(mc_input_action_held(ACTION_FORWARD),    0, "not held when key up");
    ASSERT_EQ_INT(mc_input_action_released(ACTION_FORWARD),0, "not released when key up");

    /* Frame 2: key goes down -> pressed = true, held = true */
    mock_keys[KEY_W] = 1;
    mc_input_update();
    ASSERT_EQ_INT(mc_input_action_pressed(ACTION_FORWARD), 1, "pressed on first frame down");
    ASSERT_EQ_INT(mc_input_action_held(ACTION_FORWARD),    1, "held on first frame down");
    ASSERT_EQ_INT(mc_input_action_released(ACTION_FORWARD),0, "not released when just pressed");

    /* Frame 3: key still down -> pressed = false, held = true */
    mc_input_update();
    ASSERT_EQ_INT(mc_input_action_pressed(ACTION_FORWARD), 0, "not pressed on second frame");
    ASSERT_EQ_INT(mc_input_action_held(ACTION_FORWARD),    1, "still held on second frame");
    ASSERT_EQ_INT(mc_input_action_released(ACTION_FORWARD),0, "not released while held");

    /* Frame 4: key released -> pressed = false, held = false, released = true */
    mock_keys[KEY_W] = 0;
    mc_input_update();
    ASSERT_EQ_INT(mc_input_action_pressed(ACTION_FORWARD), 0, "not pressed after release");
    ASSERT_EQ_INT(mc_input_action_held(ACTION_FORWARD),    0, "not held after release");
    ASSERT_EQ_INT(mc_input_action_released(ACTION_FORWARD),1, "released on frame of release");

    /* Frame 5: key still up -> released clears */
    mc_input_update();
    ASSERT_EQ_INT(mc_input_action_released(ACTION_FORWARD),0, "released cleared next frame");

    mc_input_shutdown();
}

/* ================================================================
 *  Tests: mouse look delta
 * ================================================================ */
static void test_mouse_look_delta(void)
{
    mc_input_init();
    mock_reset();

    mock_mouse_dx = 100.0f;
    mock_mouse_dy = -50.0f;
    mc_input_update();

    float yaw = 0.0f, pitch = 0.0f;
    mc_input_get_look_delta(&yaw, &pitch);

    /* sensitivity defaults to 0.002 */
    ASSERT_FLOAT_EQ(yaw,   100.0f * 0.002f, "yaw scaled by sensitivity");
    ASSERT_FLOAT_EQ(pitch, -50.0f * 0.002f, "pitch scaled by sensitivity");

    mc_input_shutdown();
}

/* ================================================================
 *  Tests: movement axes
 * ================================================================ */
static void test_movement_axes(void)
{
    mc_input_init();
    mock_reset();

    /* No keys -> 0 */
    mc_input_update();
    ASSERT_FLOAT_EQ(mc_input_get_move_forward(), 0.0f, "no keys = 0 forward");
    ASSERT_FLOAT_EQ(mc_input_get_move_strafe(),  0.0f, "no keys = 0 strafe");

    /* Forward only */
    mock_keys[KEY_W] = 1;
    mc_input_update();
    ASSERT_FLOAT_EQ(mc_input_get_move_forward(), 1.0f,  "W = +1 forward");
    ASSERT_FLOAT_EQ(mc_input_get_move_strafe(),  0.0f,  "W = 0 strafe");

    /* Forward + backward cancel */
    mock_keys[KEY_S] = 1;
    mc_input_update();
    ASSERT_FLOAT_EQ(mc_input_get_move_forward(), 0.0f, "W+S = 0 forward");

    /* Left only */
    mock_keys[KEY_W] = 0;
    mock_keys[KEY_S] = 0;
    mock_keys[KEY_A] = 1;
    mc_input_update();
    ASSERT_FLOAT_EQ(mc_input_get_move_strafe(), -1.0f, "A = -1 strafe");

    /* Right only */
    mock_keys[KEY_A] = 0;
    mock_keys[KEY_D] = 1;
    mc_input_update();
    ASSERT_FLOAT_EQ(mc_input_get_move_strafe(), 1.0f, "D = +1 strafe");

    mc_input_shutdown();
}

/* ================================================================
 *  Tests: out-of-range action safety
 * ================================================================ */
static void test_out_of_range_action(void)
{
    mc_input_init();
    mock_reset();
    mc_input_update();

    ASSERT_EQ_INT(mc_input_action_pressed(ACTION_COUNT),  0, "pressed OOB returns 0");
    ASSERT_EQ_INT(mc_input_action_held(ACTION_COUNT),     0, "held OOB returns 0");
    ASSERT_EQ_INT(mc_input_action_released(ACTION_COUNT), 0, "released OOB returns 0");
    ASSERT_EQ_INT(mc_input_get_binding(ACTION_COUNT),    -1, "binding OOB returns -1");

    mc_input_shutdown();
}

/* ================================================================
 *  Runner
 * ================================================================ */
int main(void)
{
    printf("mc_input tests\n");

    test_default_bindings();
    test_rebind_key();
    test_action_pressed();
    test_mouse_look_delta();
    test_movement_axes();
    test_out_of_range_action();

    printf("%d / %d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
