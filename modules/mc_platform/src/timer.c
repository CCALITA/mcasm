#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "mc_platform.h"

static double g_last_time = 0.0;
static float  g_delta     = 0.016f;

double mc_platform_get_time(void) {
    return glfwGetTime();
}

float mc_platform_get_delta(void) {
    return g_delta;
}

void mc_platform_update_timer(void) {
    double now = glfwGetTime();
    float  dt  = (float)(now - g_last_time);

    /* Clamp to avoid huge spikes (e.g. after breakpoint or suspend). */
    if (dt > 0.25f) {
        dt = 0.25f;
    }
    if (dt < 0.0f) {
        dt = 0.0f;
    }

    g_delta     = dt;
    g_last_time = now;
}
