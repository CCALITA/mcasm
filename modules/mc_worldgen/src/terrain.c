/*
 * terrain.c -- Terrain height map generation and block filling.
 *
 * Uses multi-octave 2D noise to compute a surface height for each
 * (world_x, world_z) column, then looks up the biome and adjusts:
 *   - Height multiplier (mountains = high, ocean/swamp = low)
 *   - Surface block type (grass, sand, snow)
 *   - Water fill up to sea level for ocean/swamp
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
#define SEA_LEVEL         63

/* Desert-specific constants. */
#define SAND_DEPTH        4

/* Seed offsets to decorrelate from biome noise. */
#define TERRAIN_OFFSET_X  1234.0f
#define TERRAIN_OFFSET_Z  5678.0f

void terrain_init(uint32_t seed)
{
    (void)seed;
}

/* Raw noise value at a world coordinate (not yet biome-adjusted). */
static float terrain_noise_at(int32_t world_x, int32_t world_z)
{
    float nx = ((float)world_x + TERRAIN_OFFSET_X) / HEIGHT_SCALE;
    float nz = ((float)world_z + TERRAIN_OFFSET_Z) / HEIGHT_SCALE;
    return noise_octave2d(nx, nz, 6, 0.5f, 2.0f);
}

/* Compute surface height at a world coordinate, adjusted for biome. */
static int terrain_height_for_biome(float noise_val, uint8_t biome)
{
    int h;

    switch (biome) {
    case BIOME_OCEAN:
        /* Ocean floor: lower base, reduced amplitude. */
        h = 40 + (int)(noise_val * 8.0f);
        break;
    case BIOME_MOUNTAINS:
        /* Mountains: higher base, much larger amplitude. */
        h = 80 + (int)(noise_val * 50.0f);
        break;
    case BIOME_SWAMP:
        /* Swamp: flat near sea level. */
        h = SEA_LEVEL - 2 + (int)(noise_val * 4.0f);
        break;
    default:
        /* Plains, forest, desert, tundra, taiga, savanna, jungle. */
        h = BASE_HEIGHT + (int)(noise_val * HEIGHT_AMPLITUDE);
        break;
    }

    if (h < BEDROCK_MAX) {
        h = BEDROCK_MAX;
    }
    int max_y = SECTION_COUNT * CHUNK_SIZE_Y - 1;
    if (h > max_y) {
        h = max_y;
    }
    return h;
}

/* Determine the block id for a given y in a column with the specified
 * biome and surface height.  Bedrock is always at y < BEDROCK_MAX. */
static block_id_t block_for_column(int y, int surface, uint8_t biome)
{
    if (y < BEDROCK_MAX) {
        return BLOCK_BEDROCK;
    }

    switch (biome) {
    case BIOME_DESERT:
        return (y > surface - SAND_DEPTH) ? BLOCK_SAND : BLOCK_STONE;

    case BIOME_OCEAN:
        return (y < surface - 2) ? BLOCK_STONE : BLOCK_GRAVEL;

    default:
        if (y < surface - DIRT_DEPTH) return BLOCK_STONE;
        if (y < surface)              return BLOCK_DIRT;
        return BLOCK_GRASS;
    }
}

/* Fill a single column with blocks appropriate for its biome. */
static void fill_column(chunk_t *out, int lx, int lz,
                         int surface, uint8_t biome)
{
    for (int y = 0; y <= surface; y++) {
        chunk_set_block_unchecked(out, lx, y, lz,
                                  block_for_column(y, surface, biome));
    }

    /* Tundra: add snow layer on top of grass. */
    if (biome == BIOME_TUNDRA) {
        int snow_y = surface + 1;
        if (snow_y < SECTION_COUNT * CHUNK_SIZE_Y) {
            chunk_set_block_unchecked(out, lx, snow_y, lz, BLOCK_SNOW);
        }
    }

    /* Fill water from surface+1 up to sea level for ocean and swamp. */
    if (biome == BIOME_OCEAN || biome == BIOME_SWAMP) {
        for (int y = surface + 1; y <= SEA_LEVEL; y++) {
            chunk_set_block_unchecked(out, lx, y, lz, BLOCK_WATER);
        }
    }
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

            uint8_t biome   = biome_get(world_x, world_z);
            float noise_val = terrain_noise_at(world_x, world_z);
            int surface     = terrain_height_for_biome(noise_val, biome);

            out->heightmap[lz * CHUNK_SIZE_X + lx] = surface;

            fill_column(out, lx, lz, surface, biome);
        }
    }

    out->dirty_mask = 0xFFFFFFFF;
}
