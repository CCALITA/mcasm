/*
 * terrain.c -- Terrain height map generation and block filling.
 *
 * Uses multi-octave 2D noise to compute a surface height for each
 * (world_x, world_z) column, then fills the chunk with:
 *   - Bedrock  at y < 5
 *   - Stone    below surface
 *   - Dirt     in the 3 blocks below the surface
 *   - Grass    at the surface
 *   - Air      above the surface
 *
 * The heightmap stored in chunk_t is also populated.
 */

#include "worldgen_internal.h"
#include "mc_worldgen.h"
#include <string.h>

/* Noise parameters for the primary height field. */
#define HEIGHT_SCALE      64.0f
#define HEIGHT_AMPLITUDE  20.0f
#define BASE_HEIGHT       64
#define BEDROCK_MAX       5
#define DIRT_DEPTH        3

/* Seed offsets to decorrelate from biome noise. */
#define TERRAIN_OFFSET_X  1234.0f
#define TERRAIN_OFFSET_Z  5678.0f

void terrain_init(uint32_t seed)
{
    (void)seed;
}

/* Compute surface height at a world coordinate. */
static int terrain_height_at(int32_t world_x, int32_t world_z)
{
    float nx = ((float)world_x + TERRAIN_OFFSET_X) / HEIGHT_SCALE;
    float nz = ((float)world_z + TERRAIN_OFFSET_Z) / HEIGHT_SCALE;
    float n = noise_octave2d(nx, nz, 6, 0.5f, 2.0f);
    int h = BASE_HEIGHT + (int)(n * HEIGHT_AMPLITUDE);
    if (h < BEDROCK_MAX) {
        h = BEDROCK_MAX;
    }
    int max_y = SECTION_COUNT * CHUNK_SIZE_Y - 1;
    if (h > max_y) {
        h = max_y;
    }
    return h;
}

void terrain_generate_chunk(chunk_pos_t pos, chunk_t *out)
{
    memset(out, 0, sizeof(*out));
    out->pos   = pos;
    out->state = CHUNK_STATE_GENERATING;

    int base_x = pos.x * CHUNK_SIZE_X;
    int base_z = pos.z * CHUNK_SIZE_Z;

    for (int lx = 0; lx < CHUNK_SIZE_X; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE_Z; lz++) {
            int world_x = base_x + lx;
            int world_z = base_z + lz;
            int surface = terrain_height_at(world_x, world_z);

            out->heightmap[lz * CHUNK_SIZE_X + lx] = surface;

            /* Fill only up to the surface; above is already AIR from memset. */
            for (int y = 0; y <= surface; y++) {
                block_id_t id;
                if (y < BEDROCK_MAX) {
                    id = BLOCK_BEDROCK;
                } else if (y < surface - DIRT_DEPTH) {
                    id = BLOCK_STONE;
                } else if (y < surface) {
                    id = BLOCK_DIRT;
                } else {
                    id = BLOCK_GRASS;
                }
                chunk_set_block_unchecked(out, lx, y, lz, id);
            }
        }
    }

    out->dirty_mask = 0xFFFFFFFF;
}
