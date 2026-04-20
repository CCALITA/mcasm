#ifndef WORLDGEN_INTERNAL_H
#define WORLDGEN_INTERNAL_H

#include "mc_types.h"
#include "mc_block.h"
#include <stdint.h>

/* ---- Shared block helpers ---- */

static inline block_id_t chunk_get_block(const chunk_t *c, int lx, int y, int lz)
{
    int sec_idx = y / CHUNK_SIZE_Y;
    int local_y = y % CHUNK_SIZE_Y;
    return c->sections[sec_idx].blocks[local_y * 256 + lz * 16 + lx];
}

/* Set block with bounds checking; only writes into air. Returns 1 if placed. */
static inline int chunk_place_if_air(chunk_t *c, int lx, int y, int lz, block_id_t id)
{
    if (lx < 0 || lx >= CHUNK_SIZE_X || lz < 0 || lz >= CHUNK_SIZE_Z) {
        return 0;
    }
    if (y < 0 || y >= SECTION_COUNT * CHUNK_SIZE_Y) {
        return 0;
    }
    int sec_idx = y / CHUNK_SIZE_Y;
    int local_y = y % CHUNK_SIZE_Y;
    int index   = local_y * 256 + lz * 16 + lx;
    chunk_section_t *sec = &c->sections[sec_idx];
    if (sec->blocks[index] != BLOCK_AIR) {
        return 0;
    }
    sec->blocks[index] = id;
    if (id != BLOCK_AIR) {
        sec->non_air_count++;
    }
    return 1;
}

/* Unchecked block set for initial chunk fill (caller guarantees valid coords). */
static inline void chunk_set_block_unchecked(chunk_t *c, int lx, int y, int lz, block_id_t id)
{
    int sec_idx = y / CHUNK_SIZE_Y;
    int local_y = y % CHUNK_SIZE_Y;
    int index   = local_y * 256 + lz * 16 + lx;
    c->sections[sec_idx].blocks[index] = id;
    if (id != BLOCK_AIR) {
        c->sections[sec_idx].non_air_count++;
    }
}

/* Carve a block to air if it is not already air. */
static inline void chunk_carve_to_air(chunk_t *c, int lx, int y, int lz)
{
    int sec_idx = y / CHUNK_SIZE_Y;
    int local_y = y % CHUNK_SIZE_Y;
    int index   = local_y * 256 + lz * 16 + lx;
    chunk_section_t *sec = &c->sections[sec_idx];
    if (sec->blocks[index] != BLOCK_AIR) {
        sec->blocks[index] = BLOCK_AIR;
        sec->non_air_count--;
    }
}

/* ---- Noise ---- */

void  noise_init(uint32_t seed);

/* 2D Perlin noise returning value in [-1, 1] */
float noise_perlin2d(float x, float y);

/* 3D Perlin noise returning value in [-1, 1] */
float noise_perlin3d(float x, float y, float z);

/* Multi-octave 2D fractal noise */
float noise_octave2d(float x, float y, int octaves, float persistence, float lacunarity);

/* Multi-octave 3D fractal noise */
float noise_octave3d(float x, float y, float z, int octaves, float persistence, float lacunarity);

/* ---- Biome ---- */

void    biome_init(uint32_t seed);
uint8_t biome_get(int32_t x, int32_t z);
float   biome_temperature(int32_t x, int32_t z);
float   biome_humidity(int32_t x, int32_t z);

/* ---- Terrain ---- */

void       terrain_init(uint32_t seed);
void       terrain_generate_chunk(chunk_pos_t pos, chunk_t *out);

/* ---- Caves ---- */

void caves_init(uint32_t seed);
void caves_carve(chunk_pos_t pos, chunk_t *chunk);

/* ---- Ores ---- */

void ores_init(uint32_t seed);
void ores_place(chunk_pos_t pos, chunk_t *chunk);

/* ---- Trees ---- */

void trees_init(uint32_t seed);
void trees_place(chunk_pos_t pos, chunk_t *chunk);

#endif /* WORLDGEN_INTERNAL_H */
