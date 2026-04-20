#include "input_internal.h"

void mc_input_set_default_bindings(void)
{
    g_input.bindings[ACTION_FORWARD]   = KEY_W;
    g_input.bindings[ACTION_BACKWARD]  = KEY_S;
    g_input.bindings[ACTION_LEFT]      = KEY_A;
    g_input.bindings[ACTION_RIGHT]     = KEY_D;
    g_input.bindings[ACTION_JUMP]      = KEY_SPACE;
    g_input.bindings[ACTION_SNEAK]     = KEY_LEFT_SHIFT;
    g_input.bindings[ACTION_SPRINT]    = KEY_LEFT_CONTROL;
    g_input.bindings[ACTION_DIG]       = 0;  /* mouse button — handled elsewhere */
    g_input.bindings[ACTION_PLACE]     = 0;  /* mouse button — handled elsewhere */
    g_input.bindings[ACTION_USE]       = 0;  /* mouse button — handled elsewhere */
    g_input.bindings[ACTION_INVENTORY] = KEY_E;
    g_input.bindings[ACTION_CHAT]      = KEY_T;
    g_input.bindings[ACTION_PAUSE]     = KEY_ESCAPE;
    g_input.bindings[ACTION_DEBUG]     = KEY_F3;

    /* Hotbar slots 1-9 */
    for (int i = 0; i < 9; i++) {
        g_input.bindings[ACTION_HOTBAR_1 + i] = KEY_1 + i;
    }
}

mc_error_t mc_input_bind_key(uint8_t action, int key)
{
    if (action >= ACTION_COUNT) { return MC_ERR_INVALID_ARG; }
    if (key < 0)               { return MC_ERR_INVALID_ARG; }
    g_input.bindings[action] = key;
    return MC_OK;
}

int mc_input_get_binding(uint8_t action)
{
    if (action >= ACTION_COUNT) { return -1; }
    return g_input.bindings[action];
}
