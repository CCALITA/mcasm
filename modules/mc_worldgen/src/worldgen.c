/*
 * worldgen.c -- Public API for mc_worldgen module.
 *
 * Delegates to the internal subsystems: noise, terrain, biome, caves, trees.
 */

#include "mc_worldgen.h"
#include "worldgen_internal.h"

mc_error_t mc_worldgen_init(uint32_t seed)
{
    noise_init(seed);
    terrain_init(seed);
    biome_init(seed);
    caves_init(seed);
    trees_init(seed);
    return MC_OK;
}

void mc_worldgen_shutdown(void)
{
    /* No dynamic resources to free. */
}

mc_error_t mc_worldgen_generate_chunk(chunk_pos_t pos, chunk_t *out_chunk)
{
    if (!out_chunk) {
        return MC_ERR_INVALID_ARG;
    }

    /* 1. Base terrain (heightmap + stone/dirt/grass/bedrock) */
    terrain_generate_chunk(pos, out_chunk);

    /* 2. Carve caves */
    caves_carve(pos, out_chunk);

    /* 3. Place trees */
    trees_place(pos, out_chunk);

    out_chunk->state = CHUNK_STATE_READY;
    return MC_OK;
}

uint8_t mc_worldgen_get_biome(int32_t x, int32_t z)
{
    return biome_get(x, z);
}

float mc_worldgen_get_temperature(int32_t x, int32_t z)
{
    return biome_temperature(x, z);
}

float mc_worldgen_get_humidity(int32_t x, int32_t z)
{
    return biome_humidity(x, z);
}
