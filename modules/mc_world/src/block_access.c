#include "mc_world.h"
#include "world_internal.h"
#include <string.h>
#include <math.h>

/*
 * Convert world block coordinate to chunk/section coordinate.
 * floor(v / 16) — arithmetic right shift gives floor division for negative values.
 */
static int32_t to_chunk_coord(int32_t v)
{
    return v >> 4;
}

block_id_t mc_world_get_block(block_pos_t pos)
{
    int32_t chunk_x = to_chunk_coord(pos.x);
    int32_t chunk_z = to_chunk_coord(pos.z);
    int32_t section_y = to_chunk_coord(pos.y);

    if (section_y < 0 || section_y >= SECTION_COUNT) {
        return 0;
    }

    chunk_pos_t cpos = {chunk_x, chunk_z};
    const chunk_t *chunk = world_find_chunk(cpos);
    if (!chunk) {
        return 0;
    }

    int32_t lx = pos.x & 15;
    int32_t ly = pos.y & 15;
    int32_t lz = pos.z & 15;
    int32_t index = ly * 256 + lz * 16 + lx;

    return chunk->sections[section_y].blocks[index];
}

mc_error_t mc_world_set_block(block_pos_t pos, block_id_t block)
{
    if (!g_world.initialized) {
        return MC_ERR_NOT_READY;
    }

    int32_t chunk_x = to_chunk_coord(pos.x);
    int32_t chunk_z = to_chunk_coord(pos.z);
    int32_t section_y = to_chunk_coord(pos.y);

    if (section_y < 0 || section_y >= SECTION_COUNT) {
        return MC_ERR_INVALID_ARG;
    }

    chunk_pos_t cpos = {chunk_x, chunk_z};
    chunk_t *chunk = world_find_chunk(cpos);
    if (!chunk) {
        return MC_ERR_NOT_FOUND;
    }

    int32_t lx = pos.x & 15;
    int32_t ly = pos.y & 15;
    int32_t lz = pos.z & 15;
    int32_t index = ly * 256 + lz * 16 + lx;

    block_id_t old = chunk->sections[section_y].blocks[index];
    chunk->sections[section_y].blocks[index] = block;

    /* Update non_air_count */
    if (old == 0 && block != 0) {
        chunk->sections[section_y].non_air_count++;
    } else if (old != 0 && block == 0) {
        chunk->sections[section_y].non_air_count--;
    }

    /* Mark section dirty */
    chunk->dirty_mask |= (1u << (uint32_t)section_y);

    return MC_OK;
}

mc_error_t mc_world_fill_section(chunk_pos_t cpos, uint8_t section_y,
                                  const block_id_t blocks[BLOCKS_PER_SECTION])
{
    if (!g_world.initialized) {
        return MC_ERR_NOT_READY;
    }

    if (section_y >= SECTION_COUNT || !blocks) {
        return MC_ERR_INVALID_ARG;
    }

    chunk_t *chunk = world_find_chunk(cpos);
    if (!chunk) {
        return MC_ERR_NOT_FOUND;
    }

    memcpy(chunk->sections[section_y].blocks, blocks,
           BLOCKS_PER_SECTION * sizeof(block_id_t));

    /* Recount non-air blocks */
    uint16_t count = 0;
    for (int i = 0; i < BLOCKS_PER_SECTION; i++) {
        if (blocks[i] != 0) {
            count++;
        }
    }
    chunk->sections[section_y].non_air_count = count;

    /* Mark section dirty */
    chunk->dirty_mask |= (1u << (uint32_t)section_y);

    return MC_OK;
}
