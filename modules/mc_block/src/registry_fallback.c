/*
 * registry_fallback.c -- Pure-C fallback for registry.asm
 *
 * Used on non-x86-64 architectures (e.g. Apple Silicon) where NASM
 * cannot produce native code.  Implements the same block property
 * registry as registry.asm: a static array of 256 block_properties_t
 * structs with fast lookup.
 */

#include "mc_block.h"
#include <string.h>

#ifndef __x86_64__

static block_properties_t block_registry[BLOCK_TYPE_COUNT];

mc_error_t mc_block_register(block_id_t id, const block_properties_t *props)
{
    if (id >= BLOCK_TYPE_COUNT) return MC_ERR_INVALID_ARG;
    if (!props) return MC_ERR_INVALID_ARG;

    memcpy(&block_registry[id], props, sizeof(block_properties_t));
    return MC_OK;
}

const block_properties_t *mc_block_get_properties(block_id_t id)
{
    if (id >= BLOCK_TYPE_COUNT) return NULL;
    return &block_registry[id];
}

uint8_t mc_block_is_solid(block_id_t id)
{
    if (id >= BLOCK_TYPE_COUNT) return 0;
    return block_registry[id].solid;
}

uint8_t mc_block_is_transparent(block_id_t id)
{
    if (id >= BLOCK_TYPE_COUNT) return 1;
    return block_registry[id].transparent;
}

uint8_t mc_block_get_light(block_id_t id)
{
    if (id >= BLOCK_TYPE_COUNT) return 0;
    return block_registry[id].light_emission;
}

#endif /* !__x86_64__ */
