#include "mc_world.h"
#include "world_internal.h"

int32_t mc_world_get_height(int32_t x, int32_t z)
{
    int32_t chunk_x = x >> 4;
    int32_t chunk_z = z >> 4;

    chunk_pos_t cpos = {chunk_x, chunk_z};
    const chunk_t *chunk = world_find_chunk(cpos);
    if (!chunk) {
        return 0;
    }

    int32_t lx = x & 15;
    int32_t lz = z & 15;
    return chunk->heightmap[lz * CHUNK_SIZE_X + lx];
}
