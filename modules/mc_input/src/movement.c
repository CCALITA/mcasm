#include "input_internal.h"

float mc_input_get_move_forward(void)
{
    float val = 0.0f;
    if (mc_input_action_held(ACTION_FORWARD))  { val += 1.0f; }
    if (mc_input_action_held(ACTION_BACKWARD)) { val -= 1.0f; }
    return val;
}

float mc_input_get_move_strafe(void)
{
    float val = 0.0f;
    if (mc_input_action_held(ACTION_RIGHT)) { val += 1.0f; }
    if (mc_input_action_held(ACTION_LEFT))  { val -= 1.0f; }
    return val;
}
