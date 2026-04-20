#ifndef MC_RENDER_H
#define MC_RENDER_H

#include "mc_types.h"
#include "mc_error.h"

mc_error_t mc_render_init(void* window_handle, uint32_t width, uint32_t height);
void       mc_render_shutdown(void);
mc_error_t mc_render_resize(uint32_t width, uint32_t height);

uint64_t   mc_render_upload_mesh(const chunk_mesh_t* mesh);
void       mc_render_free_mesh(uint64_t mesh_handle);
void       mc_render_update_mesh(uint64_t mesh_handle, const chunk_mesh_t* mesh);

uint64_t   mc_render_build_chunk_mesh(const chunk_section_t* section,
                                       chunk_pos_t chunk_pos, uint8_t section_y);

void       mc_render_begin_frame(const mat4_t* view, const mat4_t* projection);
void       mc_render_draw_terrain(const uint64_t* mesh_handles, uint32_t count);
void       mc_render_draw_entities(const vec3_t* positions, const uint8_t* types, uint32_t count);
void       mc_render_draw_ui(void);
void       mc_render_end_frame(void);

void       mc_render_set_clear_color(float r, float g, float b);
void       mc_render_set_fog(float start, float end, vec3_t color);

/* Day/night cycle: time_of_day is 0-24000 ticks (24000 = full cycle).
 * 0-12000 = daytime, 12000-24000 = nighttime.
 * Dawn at 0-1000, dusk at 11000-13000. */
void       mc_render_set_time(float time_of_day);
void       mc_render_advance_time(float delta_ticks);
float      mc_render_get_time(void);

#endif /* MC_RENDER_H */
