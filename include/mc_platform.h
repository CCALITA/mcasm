#ifndef MC_PLATFORM_H
#define MC_PLATFORM_H

#include "mc_types.h"
#include "mc_error.h"

mc_error_t mc_platform_init(uint32_t width, uint32_t height, const char* title);
void       mc_platform_shutdown(void);
void       mc_platform_poll_events(void);
uint8_t    mc_platform_should_close(void);
void       mc_platform_swap_buffers(void);

double     mc_platform_get_time(void);
float      mc_platform_get_delta(void);
void       mc_platform_update_timer(void);

uint8_t    mc_platform_key_pressed(int key);
uint8_t    mc_platform_key_held(int key);
void       mc_platform_get_mouse_delta(float* dx, float* dy);
void       mc_platform_get_mouse_pos(float* x, float* y);
void       mc_platform_set_cursor_locked(uint8_t locked);

void*      mc_platform_get_window_handle(void);
void       mc_platform_get_framebuffer_size(uint32_t* width, uint32_t* height);
const char** mc_platform_get_vulkan_extensions(uint32_t* count);

#endif /* MC_PLATFORM_H */
