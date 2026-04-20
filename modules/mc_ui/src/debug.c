/* debug.c -- Debug overlay (F3 screen) */

#include "mc_ui.h"
#include "ui_internal.h"
#include <stdio.h>

void mc_ui_show_debug(vec3_t pos, float fps, uint32_t chunk_count,
                      uint32_t entity_count)
{
    g_ui.debug_visible = 1;

    char buf[128];
    float y = 4.0f;
    float scale = 1.0f;
    float line_h = FONT_GLYPH_H * scale + 2.0f;

    snprintf(buf, sizeof(buf), "FPS: %.1f", (double)fps);
    mc_ui_draw_text(buf, 4.0f, y, scale);
    y += line_h;

    snprintf(buf, sizeof(buf), "Pos: %.1f / %.1f / %.1f",
             (double)pos.x, (double)pos.y, (double)pos.z);
    mc_ui_draw_text(buf, 4.0f, y, scale);
    y += line_h;

    snprintf(buf, sizeof(buf), "Chunks: %u", chunk_count);
    mc_ui_draw_text(buf, 4.0f, y, scale);
    y += line_h;

    snprintf(buf, sizeof(buf), "Entities: %u", entity_count);
    mc_ui_draw_text(buf, 4.0f, y, scale);
}
