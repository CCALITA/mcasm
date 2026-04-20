/* inventory.c -- Inventory screen: toggle, query, draw */

#include "mc_ui.h"
#include "ui_internal.h"

static void draw_slot_grid(float origin_x, float origin_y, int cols, int rows)
{
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float x0 = origin_x + (float)c * (INV_SLOT_SIZE + INV_SLOT_PAD);
            float y0 = origin_y + (float)r * (INV_SLOT_SIZE + INV_SLOT_PAD);
            ui_push_quad_px(x0, y0, x0 + INV_SLOT_SIZE, y0 + INV_SLOT_SIZE,
                            0.0f, 0.0f, 1.0f, 1.0f);
        }
    }
}

void mc_ui_toggle_inventory(void)
{
    g_ui.inventory_open = !g_ui.inventory_open;
}

uint8_t mc_ui_inventory_open(void)
{
    return g_ui.inventory_open;
}

void mc_ui_draw_inventory(void)
{
    if (!g_ui.inventory_open) return;

    float grid_w = (float)INV_COLS * (INV_SLOT_SIZE + INV_SLOT_PAD) - INV_SLOT_PAD;
    float grid_h = (float)INV_ROWS * (INV_SLOT_SIZE + INV_SLOT_PAD) - INV_SLOT_PAD;

    float origin_x = ((float)g_ui.screen_w - grid_w) * 0.5f;
    float origin_y = ((float)g_ui.screen_h - grid_h) * 0.5f + 40.0f;

    draw_slot_grid(origin_x, origin_y, INV_COLS, INV_ROWS);

    float craft_w = (float)CRAFT_COLS * (INV_SLOT_SIZE + INV_SLOT_PAD) - INV_SLOT_PAD;
    float craft_x = ((float)g_ui.screen_w - craft_w) * 0.5f;
    float craft_y = origin_y - (float)CRAFT_ROWS * (INV_SLOT_SIZE + INV_SLOT_PAD) - 20.0f;

    draw_slot_grid(craft_x, craft_y, CRAFT_COLS, CRAFT_ROWS);

    float out_x = craft_x + craft_w + INV_SLOT_SIZE;
    float out_y = craft_y + (float)(CRAFT_ROWS - 1) * (INV_SLOT_SIZE + INV_SLOT_PAD) * 0.5f;
    ui_push_quad_px(out_x, out_y, out_x + INV_SLOT_SIZE, out_y + INV_SLOT_SIZE,
                    0.0f, 0.0f, 1.0f, 1.0f);
}
