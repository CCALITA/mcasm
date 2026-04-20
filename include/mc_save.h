#ifndef MC_SAVE_H
#define MC_SAVE_H

#include "mc_types.h"
#include "mc_error.h"

mc_error_t mc_save_init(const char* world_dir);
void       mc_save_shutdown(void);

mc_error_t mc_save_chunk(chunk_pos_t pos, const chunk_t* chunk);
mc_error_t mc_save_load_chunk(chunk_pos_t pos, chunk_t* out_chunk);
uint8_t    mc_save_chunk_exists(chunk_pos_t pos);

mc_error_t mc_save_player(const char* name, vec3_t pos, float yaw, float pitch);
mc_error_t mc_save_load_player(const char* name, vec3_t* pos, float* yaw, float* pitch);

mc_error_t mc_save_world_meta(uint32_t seed, tick_t world_time, uint8_t game_mode);
mc_error_t mc_save_load_world_meta(uint32_t* seed, tick_t* world_time, uint8_t* game_mode);

#endif /* MC_SAVE_H */
