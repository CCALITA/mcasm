#include "mc_block.h"
#include <string.h>

static block_properties_t props[BLOCK_TYPE_COUNT] = {0};
mc_error_t mc_block_init(void) {
    props[BLOCK_STONE] = (block_properties_t){1.5f, 0, 1, 0, 0, 0, 0, 0};
    props[BLOCK_GRASS] = (block_properties_t){0.6f, 0, 1, 0, 0, 1, 2, 3};
    props[BLOCK_DIRT]  = (block_properties_t){0.5f, 0, 1, 0, 0, 3, 3, 3};
    return MC_OK;
}
void mc_block_shutdown(void) {}
mc_error_t mc_block_register(block_id_t id, const block_properties_t* p) { if(id>=BLOCK_TYPE_COUNT) return MC_ERR_INVALID_ARG; props[id]=*p; return MC_OK; }
const block_properties_t* mc_block_get_properties(block_id_t id) { return id<BLOCK_TYPE_COUNT ? &props[id] : NULL; }
uint8_t mc_block_is_solid(block_id_t id) { return id<BLOCK_TYPE_COUNT ? props[id].solid : 0; }
uint8_t mc_block_is_transparent(block_id_t id) { return id<BLOCK_TYPE_COUNT ? props[id].transparent : 1; }
uint8_t mc_block_get_light(block_id_t id) { return id<BLOCK_TYPE_COUNT ? props[id].light_emission : 0; }
