/*
 * trees.c -- Tree placement on grass blocks using noise-based scattering.
 *
 * Trees consist of an oak log trunk and oak leaves canopy.
 * Placement is determined by a 2D noise field: a tree is placed
 * when the noise exceeds a threshold and the surface block is grass.
 */

#include "worldgen_internal.h"

#define TREE_SCALE       8.0f
#define TREE_THRESHOLD   0.6f
#define TREE_TRUNK_H     5
#define TREE_CANOPY_R    2

/* Seed offsets. */
#define TREE_OFFSET_X   2468.0f
#define TREE_OFFSET_Z   1379.0f

void trees_init(uint32_t seed)
{
    (void)seed;
}

static void place_tree(chunk_t *chunk, int lx, int surface_y, int lz)
{
    int top_y = surface_y + TREE_TRUNK_H;
    int max_y = SECTION_COUNT * CHUNK_SIZE_Y - 1;
    if (top_y + TREE_CANOPY_R >= max_y) {
        return;
    }

    for (int y = surface_y + 1; y <= top_y; y++) {
        chunk_place_if_air(chunk, lx, y, lz, BLOCK_OAK_LOG);
    }

    int canopy_center_y = top_y;
    for (int dy = -TREE_CANOPY_R; dy <= TREE_CANOPY_R; dy++) {
        for (int dx = -TREE_CANOPY_R; dx <= TREE_CANOPY_R; dx++) {
            for (int dz = -TREE_CANOPY_R; dz <= TREE_CANOPY_R; dz++) {
                if (dx * dx + dy * dy + dz * dz > TREE_CANOPY_R * TREE_CANOPY_R + 1) {
                    continue;
                }
                if (dx == 0 && dz == 0 && dy <= 0) {
                    continue;
                }
                chunk_place_if_air(chunk, lx + dx, canopy_center_y + dy, lz + dz, BLOCK_OAK_LEAVES);
            }
        }
    }
}

void trees_place(chunk_pos_t pos, chunk_t *chunk)
{
    int base_x = pos.x * CHUNK_SIZE_X;
    int base_z = pos.z * CHUNK_SIZE_Z;

    /* Keep a minimum spacing by only checking every 4 blocks and using
       noise to decide.  This avoids trees overlapping too densely. */
    for (int lx = 2; lx < CHUNK_SIZE_X - 2; lx += 4) {
        for (int lz = 2; lz < CHUNK_SIZE_Z - 2; lz += 4) {
            int world_x = base_x + lx;
            int world_z = base_z + lz;

            float nx = ((float)world_x + TREE_OFFSET_X) / TREE_SCALE;
            float nz = ((float)world_z + TREE_OFFSET_Z) / TREE_SCALE;

            float val = noise_perlin2d(nx, nz);
            if (val < TREE_THRESHOLD) {
                continue;
            }

            /* Find surface from heightmap. */
            int surface = chunk->heightmap[lz * CHUNK_SIZE_X + lx];
            if (surface <= 0 || surface >= SECTION_COUNT * CHUNK_SIZE_Y - TREE_TRUNK_H - TREE_CANOPY_R) {
                continue;
            }

            /* Only place on grass. */
            if (chunk_get_block(chunk, lx, surface, lz) != BLOCK_GRASS) {
                continue;
            }

            place_tree(chunk, lx, surface, lz);
        }
    }
}
