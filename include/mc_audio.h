#ifndef MC_AUDIO_H
#define MC_AUDIO_H

#include "mc_types.h"
#include "mc_error.h"

mc_error_t mc_audio_init(void);
void       mc_audio_shutdown(void);

uint32_t   mc_audio_load_sound(const char* path);
void       mc_audio_play(uint32_t sound_id, vec3_t position, float volume, float pitch);
void       mc_audio_play_music(const char* path, float volume);
void       mc_audio_stop_music(void);
void       mc_audio_set_listener(vec3_t position, vec3_t forward, vec3_t up);
void       mc_audio_set_master_volume(float volume);
void       mc_audio_tick(void);

#endif /* MC_AUDIO_H */
