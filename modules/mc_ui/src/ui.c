/* ui.c -- UI system core: init, shutdown, resize, render */

#include "mc_ui.h"
#include "ui_internal.h"
#include <string.h>

/* Module-wide state */
ui_state_t g_ui;

mc_error_t mc_ui_init(uint32_t screen_width, uint32_t screen_height)
{
    if (screen_width == 0 || screen_height == 0)
        return MC_ERR_INVALID_ARG;

    memset(&g_ui, 0, sizeof(g_ui));
    g_ui.screen_w = screen_width;
    g_ui.screen_h = screen_height;
    return MC_OK;
}

void mc_ui_shutdown(void)
{
    memset(&g_ui, 0, sizeof(g_ui));
}

void mc_ui_resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0) return;
    g_ui.screen_w = width;
    g_ui.screen_h = height;
}

void mc_ui_render(void)
{
    /* Reset vertex buffer so the next frame starts fresh.
       Actual Vulkan submission will be added when mc_render integrates the UI pipeline. */
    g_ui.vert_count = 0;
}

uint32_t mc_ui_get_vertices(void* out_buffer, uint32_t max_bytes) {
    (void)out_buffer; (void)max_bytes;
    return 0;
}
