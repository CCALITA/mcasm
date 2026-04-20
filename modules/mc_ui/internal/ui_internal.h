#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H

#include "mc_types.h"
#include "mc_error.h"
#include <stdint.h>

/* ---- UI vertex: position (x,y) + texture coordinate (u,v) -------------- */
typedef struct {
    float x, y;
    float u, v;
} ui_vertex_t;

/* ---- Quad helper: 6 vertices (2 triangles) per quad -------------------- */
#define VERTS_PER_QUAD 6

/* Maximum number of quads the UI system can buffer per frame. */
#define MAX_UI_QUADS   4096
#define MAX_UI_VERTS   (MAX_UI_QUADS * VERTS_PER_QUAD)

/* ---- Bitmap font atlas layout ------------------------------------------ */
#define FONT_ATLAS_COLS   16
#define FONT_ATLAS_ROWS   8
#define FONT_GLYPH_W      8.0f
#define FONT_GLYPH_H      12.0f
#define FONT_FIRST_CHAR   32   /* space */
#define FONT_LAST_CHAR    127  /* DEL (exclusive) */

/* ---- Inventory grid constants ------------------------------------------ */
#define INV_COLS          9
#define INV_ROWS          4
#define INV_SLOT_SIZE     36.0f
#define INV_SLOT_PAD      4.0f
#define CRAFT_COLS        3
#define CRAFT_ROWS        3

/* ---- Chat constants ---------------------------------------------------- */
#define CHAT_MAX_LINES    10
#define CHAT_LINE_HEIGHT  14.0f
#define CHAT_X            4.0f
#define CHAT_BASE_Y       60.0f

/* ---- HUD constants ----------------------------------------------------- */
#define HOTBAR_SLOT_SIZE  40.0f
#define HOTBAR_PAD        4.0f
#define ICON_SIZE         16.0f
#define ICON_PAD          2.0f
#define CROSSHAIR_HALF    12.0f
#define CROSSHAIR_THICK   2.0f

/* UV third for icon fill states (full / half / empty) */
#define ICON_UV_THIRD     0.333f

/* ---- Global UI state (module-private) ---------------------------------- */
typedef struct {
    uint32_t    screen_w;
    uint32_t    screen_h;
    uint8_t     inventory_open;
    uint8_t     debug_visible;

    ui_vertex_t verts[MAX_UI_VERTS];
    uint32_t    vert_count;
} ui_state_t;

/* Single module-wide state instance (defined in ui.c) */
extern ui_state_t g_ui;

/* ---- Shared helper: push a single quad --------------------------------- */
static inline void ui_push_quad(float x0, float y0, float x1, float y1,
                                float u0, float v0, float u1, float v1)
{
    if (g_ui.vert_count + VERTS_PER_QUAD > MAX_UI_VERTS) return;
    ui_vertex_t *v = &g_ui.verts[g_ui.vert_count];

    /* triangle 1 */
    v[0] = (ui_vertex_t){ x0, y0, u0, v0 };
    v[1] = (ui_vertex_t){ x1, y0, u1, v0 };
    v[2] = (ui_vertex_t){ x0, y1, u0, v1 };
    /* triangle 2 */
    v[3] = (ui_vertex_t){ x1, y0, u1, v0 };
    v[4] = (ui_vertex_t){ x1, y1, u1, v1 };
    v[5] = (ui_vertex_t){ x0, y1, u0, v1 };

    g_ui.vert_count += VERTS_PER_QUAD;
}

/* ---- Coordinate helpers ------------------------------------------------ */

/* Convert pixel coordinates to normalised device coordinates [-1, 1]. */
static inline float ui_ndc_x(float px) {
    return (2.0f * px / (float)g_ui.screen_w) - 1.0f;
}
static inline float ui_ndc_y(float py) {
    return 1.0f - (2.0f * py / (float)g_ui.screen_h);
}

/* Push a quad in pixel space; internally converts to NDC.
   UV 0,0..1,1 means "solid colour" (renderer maps that to white). */
static inline void ui_push_quad_px(float px0, float py0, float px1, float py1,
                                   float u0, float v0, float u1, float v1)
{
    ui_push_quad(ui_ndc_x(px0), ui_ndc_y(py0),
                 ui_ndc_x(px1), ui_ndc_y(py1),
                 u0, v0, u1, v1);
}

#endif /* UI_INTERNAL_H */
