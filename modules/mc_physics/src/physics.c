#include "mc_physics.h"

static mc_block_query_fn g_block_query = NULL;

mc_block_query_fn mc_physics_get_query_fn(void) {
    return g_block_query;
}

mc_error_t mc_physics_init(mc_block_query_fn query_fn) {
    if (!query_fn) {
        return MC_ERR_INVALID_ARG;
    }
    g_block_query = query_fn;
    return MC_OK;
}

void mc_physics_shutdown(void) {
    g_block_query = NULL;
}

void mc_physics_tick(float dt) {
    (void)dt;
}
