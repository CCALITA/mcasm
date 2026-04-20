#include "input_internal.h"
#include "mc_platform.h"
#include <string.h>

/* Global input state */
input_state_t g_input;

mc_error_t mc_input_init(void)
{
    memset(&g_input, 0, sizeof(g_input));
    g_input.sensitivity = 0.002f;
    mc_input_set_default_bindings();
    return MC_OK;
}

void mc_input_shutdown(void)
{
    memset(&g_input, 0, sizeof(g_input));
}

void mc_input_update(void)
{
    for (uint8_t i = 0; i < ACTION_COUNT; i++) {
        uint8_t prev = (g_input.action_state[i] & STATE_HELD) ? STATE_PREV : 0;
        uint8_t curr = mc_platform_key_held(g_input.bindings[i]) ? STATE_HELD : 0;
        g_input.action_state[i] = prev | curr;
    }

    mc_platform_get_mouse_delta(&g_input.mouse_dx, &g_input.mouse_dy);
}

uint8_t mc_input_action_pressed(uint8_t action)
{
    if (action >= ACTION_COUNT) { return 0; }
    uint8_t s = g_input.action_state[action];
    return (s & STATE_HELD) && !(s & STATE_PREV);
}

uint8_t mc_input_action_held(uint8_t action)
{
    if (action >= ACTION_COUNT) { return 0; }
    return (g_input.action_state[action] & STATE_HELD) ? 1 : 0;
}

uint8_t mc_input_action_released(uint8_t action)
{
    if (action >= ACTION_COUNT) { return 0; }
    uint8_t s = g_input.action_state[action];
    return !(s & STATE_HELD) && (s & STATE_PREV);
}
