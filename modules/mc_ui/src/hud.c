/* hud.c -- HUD elements: crosshair, hotbar, health, hunger
 *
 * All functions produce vertex data (position + UV quads) that
 * mc_ui_render() collects from g_ui.verts[].
 *
 * Colour encoding via UV ranges (interpreted by the fragment shader):
 *   (0,0)..(1,1)   -> solid white   (crosshair, text)
 *   (0,0)..(0.5,1) -> dark gray     (slot background)
 *   (0.5,0)..(1,1) -> light gray    (selected slot highlight)
 *   icon rows use ICON_UV_THIRD slices for full / half / empty
 */

#include "mc_ui.h"
#include "ui_internal.h"

/* ---------- colour-encoded UV constants ---------------------------------- */

/* Hotbar slot: dark gray background */
#define SLOT_BG_U0   0.0f
#define SLOT_BG_V0   0.0f
#define SLOT_BG_U1   0.5f
#define SLOT_BG_V1   1.0f

/* Hotbar slot: lighter border for unselected */
#define SLOT_BORDER_U0   0.25f
#define SLOT_BORDER_V0   0.0f
#define SLOT_BORDER_U1   0.75f
#define SLOT_BORDER_V1   1.0f

/* Hotbar slot: highlighted border for selected */
#define SLOT_SEL_U0   0.5f
#define SLOT_SEL_V0   0.0f
#define SLOT_SEL_U1   1.0f
#define SLOT_SEL_V1   1.0f

/* ---- Slot border thickness ---------------------------------------------- */
#define SLOT_BORDER  2.0f

/* ---------- helpers ------------------------------------------------------ */

/* Map a fill level (how many half-points remain for this icon) to a UV offset.
   full >= 2  ->  0.0,  half >= 1  ->  1/3,  empty  ->  2/3 */
static float icon_fill_uv(float fill)
{
    if (fill >= 2.0f)      return 0.0f;
    if (fill >= 1.0f)      return ICON_UV_THIRD;
    return ICON_UV_THIRD * 2.0f;
}

/* Y position of the icon row (hearts / hunger) above the hotbar. */
static float icon_row_y(void)
{
    return (float)g_ui.screen_h - HOTBAR_SLOT_SIZE - 8.0f - ICON_SIZE - 4.0f;
}

/* ---------- Crosshair ---------------------------------------------------- */

void mc_ui_draw_crosshair(void)
{
    float cx = (float)g_ui.screen_w * 0.5f;
    float cy = (float)g_ui.screen_h * 0.5f;

    /* Horizontal bar */
    ui_push_quad_px(cx - CROSSHAIR_HALF, cy - CROSSHAIR_THICK,
                    cx + CROSSHAIR_HALF, cy + CROSSHAIR_THICK,
                    0.0f, 0.0f, 1.0f, 1.0f);

    /* Vertical bar */
    ui_push_quad_px(cx - CROSSHAIR_THICK, cy - CROSSHAIR_HALF,
                    cx + CROSSHAIR_THICK, cy + CROSSHAIR_HALF,
                    0.0f, 0.0f, 1.0f, 1.0f);
}

/* ---------- Hotbar ------------------------------------------------------- */

void mc_ui_draw_hotbar(const block_id_t *slots, uint8_t count, uint8_t selected)
{
    if (!slots || count == 0) return;

    float total_w = (float)count * HOTBAR_SLOT_SIZE
                  + (float)(count - 1) * HOTBAR_PAD;
    float start_x = ((float)g_ui.screen_w - total_w) * 0.5f;
    float y0      = (float)g_ui.screen_h - HOTBAR_SLOT_SIZE - 8.0f;

    for (uint8_t i = 0; i < count; ++i) {
        float x0 = start_x + (float)i * (HOTBAR_SLOT_SIZE + HOTBAR_PAD);
        float x1 = x0 + HOTBAR_SLOT_SIZE;
        float y1 = y0 + HOTBAR_SLOT_SIZE;

        uint8_t is_sel = (i == selected) ? 1 : 0;

        /* Border quad (full slot area) */
        float bu0 = is_sel ? SLOT_SEL_U0   : SLOT_BORDER_U0;
        float bu1 = is_sel ? SLOT_SEL_U1   : SLOT_BORDER_U1;
        float bv0 = is_sel ? SLOT_SEL_V0   : SLOT_BORDER_V0;
        float bv1 = is_sel ? SLOT_SEL_V1   : SLOT_BORDER_V1;
        ui_push_quad_px(x0, y0, x1, y1, bu0, bv0, bu1, bv1);

        /* Inner background quad (inset by border thickness) */
        ui_push_quad_px(x0 + SLOT_BORDER, y0 + SLOT_BORDER,
                        x1 - SLOT_BORDER, y1 - SLOT_BORDER,
                        SLOT_BG_U0, SLOT_BG_V0, SLOT_BG_U1, SLOT_BG_V1);

        (void)slots[i]; /* block icon rendering will be added later */
    }
}

/* ---------- Health bar --------------------------------------------------- */

void mc_ui_draw_health(float current, float max)
{
    if (max <= 0.0f) return;

    int hearts    = (int)(max * 0.5f);
    float start_x = ((float)g_ui.screen_w * 0.5f)
                  - (float)hearts * (ICON_SIZE + ICON_PAD) * 0.5f;
    float y0      = icon_row_y();

    for (int i = 0; i < hearts; ++i) {
        float x0   = start_x + (float)i * (ICON_SIZE + ICON_PAD);
        float fill = current - (float)(i * 2);
        float u0   = icon_fill_uv(fill);

        ui_push_quad_px(x0, y0, x0 + ICON_SIZE, y0 + ICON_SIZE,
                        u0, 0.0f, u0 + ICON_UV_THIRD, 1.0f);
    }
}

/* ---------- Hunger bar --------------------------------------------------- */

void mc_ui_draw_hunger(float current, float max)
{
    if (max <= 0.0f) return;

    int icons     = (int)(max * 0.5f);
    float total_w = (float)icons * (ICON_SIZE + ICON_PAD);
    float start_x = ((float)g_ui.screen_w * 0.5f) + total_w * 0.5f - ICON_SIZE;
    float y0      = icon_row_y();

    for (int i = 0; i < icons; ++i) {
        float x0   = start_x - (float)i * (ICON_SIZE + ICON_PAD);
        float fill = current - (float)(i * 2);
        float u0   = icon_fill_uv(fill);

        ui_push_quad_px(x0, y0, x0 + ICON_SIZE, y0 + ICON_SIZE,
                        u0, 0.0f, u0 + ICON_UV_THIRD, 1.0f);
    }
}
