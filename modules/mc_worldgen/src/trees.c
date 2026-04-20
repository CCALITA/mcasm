/*
 * trees.c -- Tree placement on grass blocks using noise-based scattering.
 *
 * Trees consist of an oak log trunk and oak leaves canopy.
 * Placement density varies by biome:
 *   - Forest:  dense trees (lower threshold)
 *   - Jungle:  very dense, taller trees
 *   - Desert / Ocean / Tundra: no trees
 *   - Swamp:   sparse trees
 *   - Others:  normal density
 */

#include "worldgen_internal.h"
#include "mc_worldgen.h"

#define TREE_SCALE       8.0f
#define TREE_THRESHOLD   0.6f
#define TREE_TRUNK_H     5
#define TREE_CANOPY_R    2

/* Jungle trees are taller. */
#define JUNGLE_TRUNK_H   9
#define JUNGLE_CANOPY_R  3

/* Seed offsets. */
#define TREE_OFFSET_X   2468.0f
#define TREE_OFFSET_Z   1379.0f

void trees_init(uint32_t seed)
{
    (void)seed;
}

static void place_tree(chunk_t *chunk, int lx, int surface_y, int lz,
                        int trunk_h, int canopy_r)
{
    int top_y = surface_y + trunk_h;
    int max_y = SECTION_COUNT * CHUNK_SIZE_Y - 1;
    if (top_y + canopy_r >= max_y) {
        return;
    }

    for (int y = surface_y + 1; y <= top_y; y++) {
        chunk_place_if_air(chunk, lx, y, lz, BLOCK_OAK_LOG);
    }

    int canopy_center_y = top_y;
    for (int dy = -canopy_r; dy <= canopy_r; dy++) {
        for (int dx = -canopy_r; dx <= canopy_r; dx++) {
            for (int dz = -canopy_r; dz <= canopy_r; dz++) {
                if (dx * dx + dy * dy + dz * dz > canopy_r * canopy_r + 1) {
                    continue;
                }
                if (dx == 0 && dz == 0 && dy <= 0) {
                    continue;
                }
                chunk_place_if_air(chunk, lx + dx, canopy_center_y + dy,
                                   lz + dz, BLOCK_OAK_LEAVES);
            }
        }
    }
}

/* Per-biome tree placement parameters. */
typedef struct {
    float threshold;  /* Noise threshold; 2.0 = never place. */
    int   step;       /* Grid step between candidate positions. */
    int   trunk_h;
    int   canopy_r;
} tree_params_t;

static tree_params_t tree_params_for_biome(uint8_t biome)
{
    switch (biome) {
    case BIOME_DESERT:
    case BIOME_OCEAN:
    case BIOME_TUNDRA:
        return (tree_params_t){2.0f, 4, TREE_TRUNK_H, TREE_CANOPY_R};
    case BIOME_FOREST:
        return (tree_params_t){0.2f, 3, TREE_TRUNK_H, TREE_CANOPY_R};
    case BIOME_JUNGLE:
        return (tree_params_t){0.1f, 2, JUNGLE_TRUNK_H, JUNGLE_CANOPY_R};
    case BIOME_SWAMP:
        return (tree_params_t){0.75f, 4, TREE_TRUNK_H, TREE_CANOPY_R};
    default:
        return (tree_params_t){TREE_THRESHOLD, 4, TREE_TRUNK_H, TREE_CANOPY_R};
    }
}

void trees_place(chunk_pos_t pos, chunk_t *chunk)
{
    int base_x = pos.x * CHUNK_SIZE_X;
    int base_z = pos.z * CHUNK_SIZE_Z;

    /* Sample biome at chunk center for density parameters. */
    int center_x = base_x + CHUNK_SIZE_X / 2;
    int center_z = base_z + CHUNK_SIZE_Z / 2;
    uint8_t biome = biome_get(center_x, center_z);
    tree_params_t tp = tree_params_for_biome(biome);

    int margin = (tp.canopy_r > 2) ? tp.canopy_r : 2;

    for (int lx = margin; lx < CHUNK_SIZE_X - margin; lx += tp.step) {
        for (int lz = margin; lz < CHUNK_SIZE_Z - margin; lz += tp.step) {
            int world_x = base_x + lx;
            int world_z = base_z + lz;

            float nx = ((float)world_x + TREE_OFFSET_X) / TREE_SCALE;
            float nz = ((float)world_z + TREE_OFFSET_Z) / TREE_SCALE;

            float val = noise_perlin2d(nx, nz);
            if (val < tp.threshold) {
                continue;
            }

            /* Find surface from heightmap. */
            int surface = chunk->heightmap[lz * CHUNK_SIZE_X + lx];
            if (surface <= 0 ||
                surface >= SECTION_COUNT * CHUNK_SIZE_Y - tp.trunk_h - tp.canopy_r) {
                continue;
            }

            /* Only place on grass. */
            if (chunk_get_block(chunk, lx, surface, lz) != BLOCK_GRASS) {
                continue;
            }

            place_tree(chunk, lx, surface, lz, tp.trunk_h, tp.canopy_r);
        }
    }
}
