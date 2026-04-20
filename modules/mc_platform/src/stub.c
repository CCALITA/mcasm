#include "mc_platform.h"
#include <time.h>

static double start_time = 0;
static float delta = 0.016f;
static uint8_t should_close = 0;

mc_error_t mc_platform_init(uint32_t width, uint32_t height, const char* title) {
    (void)width; (void)height; (void)title; return MC_OK;
}
void mc_platform_shutdown(void) {}
void mc_platform_poll_events(void) {}
uint8_t mc_platform_should_close(void) { return should_close; }
void mc_platform_swap_buffers(void) {}
double mc_platform_get_time(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec*1e-9 - start_time; }
float mc_platform_get_delta(void) { return delta; }
void mc_platform_update_timer(void) { delta = 0.016f; }
uint8_t mc_platform_key_pressed(int key) { (void)key; return 0; }
uint8_t mc_platform_key_held(int key) { (void)key; return 0; }
void mc_platform_get_mouse_delta(float* dx, float* dy) { *dx = 0; *dy = 0; }
void mc_platform_get_mouse_pos(float* x, float* y) { *x = 0; *y = 0; }
void mc_platform_set_cursor_locked(uint8_t locked) { (void)locked; }
void* mc_platform_get_window_handle(void) { return NULL; }
void mc_platform_get_framebuffer_size(uint32_t* w, uint32_t* h) { *w = 800; *h = 600; }
const char** mc_platform_get_vulkan_extensions(uint32_t* count) { *count = 0; return NULL; }
