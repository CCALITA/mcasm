/* text.c -- Bitmap font text rendering */

#include "mc_ui.h"
#include "ui_internal.h"

void mc_ui_draw_text(const char *text, float x, float y, float scale)
{
    if (!text) return;

    float cell_w = 1.0f / (float)FONT_ATLAS_COLS;
    float cell_h = 1.0f / (float)FONT_ATLAS_ROWS;

    float cursor_x = x;
    float glyph_w  = FONT_GLYPH_W * scale;
    float glyph_h  = FONT_GLYPH_H * scale;

    for (const char *p = text; *p != '\0'; ++p) {
        int ch = (unsigned char)*p;

        if (ch < FONT_FIRST_CHAR || ch >= FONT_LAST_CHAR) {
            cursor_x += glyph_w;
            continue;
        }

        int index = ch - FONT_FIRST_CHAR;
        int col   = index % FONT_ATLAS_COLS;
        int row   = index / FONT_ATLAS_COLS;

        float u0 = (float)col * cell_w;
        float v0 = (float)row * cell_h;
        float u1 = u0 + cell_w;
        float v1 = v0 + cell_h;

        ui_push_quad_px(cursor_x, y, cursor_x + glyph_w, y + glyph_h,
                        u0, v0, u1, v1);

        cursor_x += glyph_w;
    }
}
