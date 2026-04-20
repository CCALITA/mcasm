#include "mc_ui.h"
mc_error_t mc_ui_init(uint32_t w, uint32_t h) { (void)w;(void)h; return MC_OK; }
void mc_ui_shutdown(void) {}
void mc_ui_resize(uint32_t w, uint32_t h) { (void)w;(void)h; }
void mc_ui_draw_crosshair(void) {}
void mc_ui_draw_hotbar(const block_id_t* s, uint8_t c, uint8_t sel) { (void)s;(void)c;(void)sel; }
void mc_ui_draw_health(float c, float m) { (void)c;(void)m; }
void mc_ui_draw_hunger(float c, float m) { (void)c;(void)m; }
void mc_ui_draw_text(const char* t, float x, float y, float s) { (void)t;(void)x;(void)y;(void)s; }
void mc_ui_draw_chat(const char** m, uint32_t c) { (void)m;(void)c; }
uint8_t mc_ui_inventory_open(void) { return 0; }
void mc_ui_toggle_inventory(void) {}
void mc_ui_draw_inventory(void) {}
void mc_ui_show_debug(vec3_t p, float f, uint32_t cc, uint32_t ec) { (void)p;(void)f;(void)cc;(void)ec; }
void mc_ui_render(void) {}
