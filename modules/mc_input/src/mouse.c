#include "input_internal.h"

void mc_input_get_look_delta(float* yaw, float* pitch)
{
    if (yaw)   { *yaw   = g_input.mouse_dx * g_input.sensitivity; }
    if (pitch)  { *pitch  = g_input.mouse_dy * g_input.sensitivity; }
}
