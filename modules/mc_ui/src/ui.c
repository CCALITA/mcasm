/* ui.c -- UI system core: init, shutdown, resize, render, vertex export */

#include "mc_ui.h"
#include "ui_internal.h"
#include <string.h>
#include <stdint.h>

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
    g_ui.vert_count = 0;
}

uint32_t mc_ui_get_vertices(void *dst, uint32_t max_bytes)
{
    if (!dst || max_bytes == 0 || g_ui.vert_count == 0)
        return 0;

    uint32_t available = g_ui.vert_count;
    uint32_t byte_size = available * (uint32_t)sizeof(ui_vertex_t);

    if (byte_size > max_bytes) {
        available = max_bytes / (uint32_t)sizeof(ui_vertex_t);
        byte_size = available * (uint32_t)sizeof(ui_vertex_t);
    }

    memcpy(dst, g_ui.verts, byte_size);
    return available;
}
