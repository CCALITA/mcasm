#include "mc_world.h"
#include <string.h>

static uint32_t world_seed = 0;
mc_error_t mc_world_init(uint32_t seed) { world_seed = seed; return MC_OK; }
void mc_world_shutdown(void) {}
const chunk_t* mc_world_get_chunk(chunk_pos_t pos) { (void)pos; return NULL; }
mc_error_t mc_world_load_chunk(chunk_pos_t pos) { (void)pos; return MC_OK; }
void mc_world_unload_chunk(chunk_pos_t pos) { (void)pos; }
block_id_t mc_world_get_block(block_pos_t pos) { (void)pos; return 0; }
mc_error_t mc_world_set_block(block_pos_t pos, block_id_t b) { (void)pos;(void)b; return MC_OK; }
mc_error_t mc_world_fill_section(chunk_pos_t c, uint8_t sy, const block_id_t b[BLOCKS_PER_SECTION]) { (void)c;(void)sy;(void)b; return MC_OK; }
uint8_t mc_world_get_block_light(block_pos_t pos) { (void)pos; return 0; }
uint8_t mc_world_get_sky_light(block_pos_t pos) { (void)pos; return 15; }
void mc_world_propagate_light(chunk_pos_t c) { (void)c; }
uint32_t mc_world_loaded_chunk_count(void) { return 0; }
void mc_world_get_dirty_chunks(chunk_pos_t* o, uint32_t max, uint32_t* count) { (void)o;(void)max; *count=0; }
int32_t mc_world_get_height(int32_t x, int32_t z) { (void)x;(void)z; return 64; }
void mc_world_tick(tick_t t) { (void)t; }
