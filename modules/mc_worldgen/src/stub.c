#include "mc_worldgen.h"
#include <string.h>

static uint32_t gen_seed = 0;
mc_error_t mc_worldgen_init(uint32_t seed) { gen_seed = seed; return MC_OK; }
void mc_worldgen_shutdown(void) {}
mc_error_t mc_worldgen_generate_chunk(chunk_pos_t pos, chunk_t* out) { (void)pos; memset(out, 0, sizeof(*out)); return MC_OK; }
uint8_t mc_worldgen_get_biome(int32_t x, int32_t z) { (void)x;(void)z; return BIOME_PLAINS; }
float mc_worldgen_get_temperature(int32_t x, int32_t z) { (void)x;(void)z; return 0.5f; }
float mc_worldgen_get_humidity(int32_t x, int32_t z) { (void)x;(void)z; return 0.5f; }
