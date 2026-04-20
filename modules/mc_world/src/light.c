#include "mc_world.h"
#include "world_internal.h"

/*
 * Light stubs -- return sensible defaults for now.
 * Block light = 0 (no artificial light sources placed).
 * Sky light = 15 (full daylight everywhere).
 */

uint8_t mc_world_get_block_light(block_pos_t pos)
{
    (void)pos;
    return 0;
}

uint8_t mc_world_get_sky_light(block_pos_t pos)
{
    (void)pos;
    return 15;
}

void mc_world_propagate_light(chunk_pos_t chunk)
{
    (void)chunk;
}
