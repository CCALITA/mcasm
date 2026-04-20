#ifndef MC_WORLD_H
#define MC_WORLD_H

#include "mc_types.h"
#include "mc_error.h"

mc_error_t     mc_world_init(uint32_t seed);
void           mc_world_shutdown(void);

const chunk_t* mc_world_get_chunk(chunk_pos_t pos);
mc_error_t     mc_world_load_chunk(chunk_pos_t pos);
void           mc_world_unload_chunk(chunk_pos_t pos);

block_id_t     mc_world_get_block(block_pos_t pos);
mc_error_t     mc_world_set_block(block_pos_t pos, block_id_t block);
mc_error_t     mc_world_fill_section(chunk_pos_t chunk, uint8_t section_y,
                                      const block_id_t blocks[BLOCKS_PER_SECTION]);

uint8_t        mc_world_get_block_light(block_pos_t pos);
uint8_t        mc_world_get_sky_light(block_pos_t pos);
void           mc_world_propagate_light(chunk_pos_t chunk);

uint32_t       mc_world_loaded_chunk_count(void);
void           mc_world_get_dirty_chunks(chunk_pos_t* out, uint32_t max, uint32_t* count);
int32_t        mc_world_get_height(int32_t x, int32_t z);

void           mc_world_tick(tick_t current_tick);

#endif /* MC_WORLD_H */
