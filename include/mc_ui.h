#ifndef MC_UI_H
#define MC_UI_H

#include "mc_types.h"
#include "mc_error.h"

mc_error_t mc_ui_init(uint32_t screen_width, uint32_t screen_height);
void       mc_ui_shutdown(void);
void       mc_ui_resize(uint32_t width, uint32_t height);

void       mc_ui_draw_crosshair(void);
void       mc_ui_draw_hotbar(const block_id_t* slots, uint8_t count, uint8_t selected);
void       mc_ui_draw_health(float current, float max);
void       mc_ui_draw_hunger(float current, float max);
void       mc_ui_draw_text(const char* text, float x, float y, float scale);
void       mc_ui_draw_chat(const char** messages, uint32_t count);

uint8_t    mc_ui_inventory_open(void);
void       mc_ui_toggle_inventory(void);
void       mc_ui_draw_inventory(void);

void       mc_ui_show_debug(vec3_t pos, float fps, uint32_t chunk_count, uint32_t entity_count);

void       mc_ui_render(void);

#endif /* MC_UI_H */
