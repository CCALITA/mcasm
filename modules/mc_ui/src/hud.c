/* hud.c -- HUD elements: crosshair, hotbar, health, hunger */

#include "mc_ui.h"
#include "ui_internal.h"

/* Map a fill level (how many half-points remain for this icon) to a UV offset.
   full >= 2  ->  0.0,  half >= 1  ->  1/3,  empty  ->  2/3 */
static float icon_fill_uv(float fill)
{
    if (fill >= 2.0f)      return 0.0f;
    if (fill >= 1.0f)      return ICON_UV_THIRD;
    return ICON_UV_THIRD * 2.0f;
}

static float icon_row_y(void)
{
    return (float)g_ui.screen_h - HOTBAR_SLOT_SIZE - 8.0f - ICON_SIZE - 4.0f;
}

void mc_ui_draw_crosshair(void)
{
    float cx = (float)g_ui.screen_w * 0.5f;
    float cy = (float)g_ui.screen_h * 0.5f;

    ui_push_quad_px(cx - CROSSHAIR_HALF, cy - CROSSHAIR_THICK,
                    cx + CROSSHAIR_HALF, cy + CROSSHAIR_THICK,
                    0.0f, 0.0f, 1.0f, 1.0f);
    ui_push_quad_px(cx - CROSSHAIR_THICK, cy - CROSSHAIR_HALF,
                    cx + CROSSHAIR_THICK, cy + CROSSHAIR_HALF,
                    0.0f, 0.0f, 1.0f, 1.0f);
}

void mc_ui_draw_hotbar(const block_id_t *slots, uint8_t count, uint8_t selected)
{
    if (!slots || count == 0) return;

    float total_w = (float)count * HOTBAR_SLOT_SIZE + (float)(count - 1) * HOTBAR_PAD;
    float start_x = ((float)g_ui.screen_w - total_w) * 0.5f;
    float y0      = (float)g_ui.screen_h - HOTBAR_SLOT_SIZE - 8.0f;

    for (uint8_t i = 0; i < count; ++i) {
        float x0 = start_x + (float)i * (HOTBAR_SLOT_SIZE + HOTBAR_PAD);
        float u0 = (i == selected) ? 0.5f : 0.0f;
        ui_push_quad_px(x0, y0, x0 + HOTBAR_SLOT_SIZE, y0 + HOTBAR_SLOT_SIZE,
                        u0, 0.0f, u0 + 0.5f, 1.0f);
        (void)slots[i];
    }
}

void mc_ui_draw_health(float current, float max)
{
    if (max <= 0.0f) return;

    int hearts    = (int)(max * 0.5f);
    float start_x = ((float)g_ui.screen_w - (float)hearts * (ICON_SIZE + ICON_PAD)) * 0.5f;
    float y0      = icon_row_y();

    for (int i = 0; i < hearts; ++i) {
        float x0 = start_x + (float)i * (ICON_SIZE + ICON_PAD);
        float u0 = icon_fill_uv(current - (float)(i * 2));
        ui_push_quad_px(x0, y0, x0 + ICON_SIZE, y0 + ICON_SIZE,
                        u0, 0.0f, u0 + ICON_UV_THIRD, 1.0f);
    }
}

void mc_ui_draw_hunger(float current, float max)
{
    if (max <= 0.0f) return;

    int icons     = (int)(max * 0.5f);
    float total_w = (float)icons * (ICON_SIZE + ICON_PAD);
    float start_x = ((float)g_ui.screen_w + total_w) * 0.5f - ICON_SIZE;
    float y0      = icon_row_y();

    for (int i = 0; i < icons; ++i) {
        float x0 = start_x - (float)i * (ICON_SIZE + ICON_PAD);
        float u0 = icon_fill_uv(current - (float)(i * 2));
        ui_push_quad_px(x0, y0, x0 + ICON_SIZE, y0 + ICON_SIZE,
                        u0, 0.0f, u0 + ICON_UV_THIRD, 1.0f);
    }
}
