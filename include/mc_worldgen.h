#ifndef MC_WORLDGEN_H
#define MC_WORLDGEN_H

#include "mc_types.h"
#include "mc_error.h"

mc_error_t mc_worldgen_init(uint32_t seed);
void       mc_worldgen_shutdown(void);

mc_error_t mc_worldgen_generate_chunk(chunk_pos_t pos, chunk_t* out_chunk);

uint8_t    mc_worldgen_get_biome(int32_t x, int32_t z);
float      mc_worldgen_get_temperature(int32_t x, int32_t z);
float      mc_worldgen_get_humidity(int32_t x, int32_t z);

#define BIOME_PLAINS       0
#define BIOME_FOREST       1
#define BIOME_DESERT       2
#define BIOME_MOUNTAINS    3
#define BIOME_OCEAN        4
#define BIOME_TAIGA        5
#define BIOME_SWAMP        6
#define BIOME_JUNGLE       7
#define BIOME_SAVANNA      8
#define BIOME_TUNDRA       9
#define BIOME_COUNT        10

#endif /* MC_WORLDGEN_H */
