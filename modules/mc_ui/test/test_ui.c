/* test_ui.c -- mc_ui module tests */

#include "mc_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Expose internal state for assertions.
   The test links against the static lib, so we can extern the global. */
typedef struct {
    float x, y;
    float u, v;
} ui_vertex_t;

#define VERTS_PER_QUAD 6
#define MAX_UI_QUADS   4096
#define MAX_UI_VERTS   (MAX_UI_QUADS * VERTS_PER_QUAD)

typedef struct {
    uint32_t    screen_w;
    uint32_t    screen_h;
    uint8_t     inventory_open;
    uint8_t     debug_visible;
    ui_vertex_t verts[MAX_UI_VERTS];
    uint32_t    vert_count;
} ui_state_t;

extern ui_state_t g_ui;

/* ---- Helpers ----------------------------------------------------------- */

static int tests_run    = 0;
static int tests_passed = 0;

#define ASSERT(cond, msg)                                       \
    do {                                                        \
        tests_run++;                                            \
        if (!(cond)) {                                          \
            fprintf(stderr, "  FAIL: %s (line %d)\n", msg, __LINE__); \
        } else {                                                \
            tests_passed++;                                     \
        }                                                       \
    } while (0)

/* ---- Tests ------------------------------------------------------------- */

static void test_init_shutdown(void)
{
    mc_error_t err = mc_ui_init(800, 600);
    ASSERT(err == MC_OK, "init with valid dimensions succeeds");
    ASSERT(g_ui.screen_w == 800, "screen_w set correctly");
    ASSERT(g_ui.screen_h == 600, "screen_h set correctly");
    ASSERT(g_ui.vert_count == 0, "vert_count starts at zero");

    mc_ui_shutdown();
    ASSERT(g_ui.screen_w == 0, "shutdown clears screen_w");
    ASSERT(g_ui.screen_h == 0, "shutdown clears screen_h");
}

static void test_init_invalid(void)
{
    mc_error_t err = mc_ui_init(0, 600);
    ASSERT(err == MC_ERR_INVALID_ARG, "init rejects zero width");

    err = mc_ui_init(800, 0);
    ASSERT(err == MC_ERR_INVALID_ARG, "init rejects zero height");
}

static void test_resize(void)
{
    mc_ui_init(800, 600);
    mc_ui_resize(1920, 1080);
    ASSERT(g_ui.screen_w == 1920, "resize updates width");
    ASSERT(g_ui.screen_h == 1080, "resize updates height");
    mc_ui_shutdown();
}

static void test_inventory_toggle(void)
{
    mc_ui_init(800, 600);

    ASSERT(mc_ui_inventory_open() == 0, "inventory starts closed");

    mc_ui_toggle_inventory();
    ASSERT(mc_ui_inventory_open() == 1, "toggle opens inventory");

    mc_ui_toggle_inventory();
    ASSERT(mc_ui_inventory_open() == 0, "toggle closes inventory");

    mc_ui_shutdown();
}

static void test_text_vertex_generation(void)
{
    mc_ui_init(800, 600);

    /* Draw "Hi" -- two printable characters, each producing one quad = 6 verts */
    mc_ui_draw_text("Hi", 0.0f, 0.0f, 1.0f);
    ASSERT(g_ui.vert_count == 2 * VERTS_PER_QUAD,
           "two-char string produces 2 quads");

    /* Reset and draw with unprintable chars */
    g_ui.vert_count = 0;
    mc_ui_draw_text("A\x01Z", 0.0f, 0.0f, 1.0f);
    ASSERT(g_ui.vert_count == 2 * VERTS_PER_QUAD,
           "unprintable chars produce no quads");

    /* Null text should be safe */
    g_ui.vert_count = 0;
    mc_ui_draw_text(NULL, 0.0f, 0.0f, 1.0f);
    ASSERT(g_ui.vert_count == 0, "NULL text produces no quads");

    /* Empty string */
    mc_ui_draw_text("", 0.0f, 0.0f, 1.0f);
    ASSERT(g_ui.vert_count == 0, "empty string produces no quads");

    mc_ui_shutdown();
}

static void test_crosshair(void)
{
    mc_ui_init(800, 600);

    mc_ui_draw_crosshair();
    /* Crosshair = 2 bars = 2 quads = 12 verts */
    ASSERT(g_ui.vert_count == 2 * VERTS_PER_QUAD,
           "crosshair produces 2 quads");

    mc_ui_shutdown();
}

static void test_render_resets(void)
{
    mc_ui_init(800, 600);

    mc_ui_draw_crosshair();
    ASSERT(g_ui.vert_count > 0, "verts queued before render");

    mc_ui_render();
    ASSERT(g_ui.vert_count == 0, "render resets vert_count");

    mc_ui_shutdown();
}

static void test_hotbar(void)
{
    mc_ui_init(800, 600);

    block_id_t slots[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    mc_ui_draw_hotbar(slots, 9, 0);
    ASSERT(g_ui.vert_count == 9 * VERTS_PER_QUAD,
           "hotbar with 9 slots produces 9 quads");

    /* NULL slots should be safe */
    g_ui.vert_count = 0;
    mc_ui_draw_hotbar(NULL, 9, 0);
    ASSERT(g_ui.vert_count == 0, "NULL slots produces no quads");

    mc_ui_shutdown();
}

static void test_inventory_draw(void)
{
    mc_ui_init(800, 600);

    /* Inventory closed: no quads */
    mc_ui_draw_inventory();
    ASSERT(g_ui.vert_count == 0, "closed inventory produces no quads");

    /* Open inventory and draw */
    mc_ui_toggle_inventory();
    mc_ui_draw_inventory();
    /* 9x4 grid (36) + 3x3 crafting (9) + 1 output = 46 quads */
    ASSERT(g_ui.vert_count == 46 * VERTS_PER_QUAD,
           "open inventory produces 46 quads");

    mc_ui_shutdown();
}

static void test_chat(void)
{
    mc_ui_init(800, 600);

    const char *msgs[] = {"Hello", "World"};
    mc_ui_draw_chat(msgs, 2);
    /* Each message is drawn via mc_ui_draw_text.
       "Hello" = 5 quads, "World" = 5 quads = 10 quads total */
    ASSERT(g_ui.vert_count == 10 * VERTS_PER_QUAD,
           "2 chat messages produce expected quads");

    /* NULL messages should be safe */
    g_ui.vert_count = 0;
    mc_ui_draw_chat(NULL, 5);
    ASSERT(g_ui.vert_count == 0, "NULL messages produces no quads");

    mc_ui_shutdown();
}

/* ---- Main -------------------------------------------------------------- */

int main(void)
{
    printf("mc_ui tests\n");

    test_init_shutdown();
    test_init_invalid();
    test_resize();
    test_inventory_toggle();
    test_text_vertex_generation();
    test_crosshair();
    test_render_resets();
    test_hotbar();
    test_inventory_draw();
    test_chat();

    printf("  %d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
