/* debug.c -- Debug overlay (F3 screen)
 *
 * Each line gets a dark background quad for readability, then text on top.
 * All geometry is pushed into g_ui.verts[] via ui_push_quad_px / mc_ui_draw_text.
 */

#include "mc_ui.h"
#include "ui_internal.h"
#include <stdio.h>

/* Background quad UVs -- dark overlay behind text */
#define DBG_BG_U0  0.0f
#define DBG_BG_V0  0.0f
#define DBG_BG_U1  0.5f
#define DBG_BG_V1  0.5f

#define DBG_PAD_X    4.0f
#define DBG_PAD_Y    4.0f
#define DBG_PANEL_W  280.0f

/* Draw one debug line: background quad + text, advance cursor. */
static void dbg_line(const char *text, float line_h, float *y)
{
    ui_push_quad_px(0.0f, *y, DBG_PANEL_W, *y + line_h,
                    DBG_BG_U0, DBG_BG_V0, DBG_BG_U1, DBG_BG_V1);
    mc_ui_draw_text(text, DBG_PAD_X, *y, 1.0f);
    *y += line_h;
}

void mc_ui_show_debug(vec3_t pos, float fps, uint32_t chunk_count,
                      uint32_t entity_count)
{
    g_ui.debug_visible = 1;

    char buf[128];
    float y      = DBG_PAD_Y;
    float line_h = FONT_GLYPH_H + 2.0f;

    snprintf(buf, sizeof(buf), "FPS: %.1f", (double)fps);
    dbg_line(buf, line_h, &y);

    snprintf(buf, sizeof(buf), "Pos: %.1f / %.1f / %.1f",
             (double)pos.x, (double)pos.y, (double)pos.z);
    dbg_line(buf, line_h, &y);

    snprintf(buf, sizeof(buf), "Chunks: %u", chunk_count);
    dbg_line(buf, line_h, &y);

    snprintf(buf, sizeof(buf), "Entities: %u", entity_count);
    dbg_line(buf, line_h, &y);
}
