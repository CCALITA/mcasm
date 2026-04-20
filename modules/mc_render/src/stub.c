#include "mc_render.h"

mc_error_t mc_render_init(void* wh, uint32_t w, uint32_t h) { (void)wh;(void)w;(void)h; return MC_OK; }
void mc_render_shutdown(void) {}
mc_error_t mc_render_resize(uint32_t w, uint32_t h) { (void)w;(void)h; return MC_OK; }
uint64_t mc_render_upload_mesh(const chunk_mesh_t* m) { (void)m; return 1; }
void mc_render_free_mesh(uint64_t h) { (void)h; }
void mc_render_update_mesh(uint64_t h, const chunk_mesh_t* m) { (void)h;(void)m; }
uint64_t mc_render_build_chunk_mesh(const chunk_section_t* s, chunk_pos_t p, uint8_t y) { (void)s;(void)p;(void)y; return 1; }
void mc_render_begin_frame(const mat4_t* v, const mat4_t* p) { (void)v;(void)p; }
void mc_render_draw_terrain(const uint64_t* h, uint32_t c) { (void)h;(void)c; }
void mc_render_draw_entities(const vec3_t* p, const uint8_t* t, uint32_t c) { (void)p;(void)t;(void)c; }
void mc_render_draw_ui(void) {}
void mc_render_end_frame(void) {}
void mc_render_set_clear_color(float r, float g, float b) { (void)r;(void)g;(void)b; }
void mc_render_set_fog(float s, float e, vec3_t c) { (void)s;(void)e;(void)c; }
