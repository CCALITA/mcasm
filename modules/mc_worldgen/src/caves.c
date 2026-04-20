/*
 * caves.c -- Cave carving using 3D noise thresholds.
 *
 * A simple "cheese cave" approach: sample 3D noise at each block
 * position and carve air where the value exceeds a threshold.
 * Caves are only carved between y=10 and y=55 to keep bedrock
 * and surface intact.
 */

#include "worldgen_internal.h"

#define CAVE_SCALE       32.0f
#define CAVE_THRESHOLD   0.55f
#define CAVE_MIN_Y       10
#define CAVE_MAX_Y       55

/* Seed offsets to decorrelate cave noise. */
#define CAVE_OFFSET_X   4321.0f
#define CAVE_OFFSET_Y   8765.0f
#define CAVE_OFFSET_Z   1357.0f

void caves_init(uint32_t seed)
{
    (void)seed;
}

void caves_carve(chunk_pos_t pos, chunk_t *chunk)
{
    int base_x = pos.x * CHUNK_SIZE_X;
    int base_z = pos.z * CHUNK_SIZE_Z;

    for (int lx = 0; lx < CHUNK_SIZE_X; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE_Z; lz++) {
            int world_x = base_x + lx;
            int world_z = base_z + lz;

            for (int y = CAVE_MIN_Y; y <= CAVE_MAX_Y; y++) {
                float nx = ((float)world_x + CAVE_OFFSET_X) / CAVE_SCALE;
                float ny = ((float)y       + CAVE_OFFSET_Y) / CAVE_SCALE;
                float nz = ((float)world_z + CAVE_OFFSET_Z) / CAVE_SCALE;

                float val = noise_octave3d(nx, ny, nz, 3, 0.5f, 2.0f);

                if (val > CAVE_THRESHOLD) {
                    chunk_carve_to_air(chunk, lx, y, lz);
                }
            }
        }
    }
}
