#include "mc_audio.h"
mc_error_t mc_audio_init(void) { return MC_OK; }
void mc_audio_shutdown(void) {}
uint32_t mc_audio_load_sound(const char* p) { (void)p; return 1; }
void mc_audio_play(uint32_t id, vec3_t pos, float vol, float pitch) { (void)id;(void)pos;(void)vol;(void)pitch; }
void mc_audio_play_music(const char* p, float v) { (void)p;(void)v; }
void mc_audio_stop_music(void) {}
void mc_audio_set_listener(vec3_t p, vec3_t f, vec3_t u) { (void)p;(void)f;(void)u; }
void mc_audio_set_master_volume(float v) { (void)v; }
void mc_audio_tick(void) {}
