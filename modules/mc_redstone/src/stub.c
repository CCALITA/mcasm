#include "mc_redstone.h"
mc_error_t mc_redstone_init(mc_redstone_block_query_fn q, mc_redstone_block_set_fn s) { (void)q;(void)s; return MC_OK; }
void mc_redstone_shutdown(void) {}
uint8_t mc_redstone_get_power(block_pos_t p) { (void)p; return 0; }
void mc_redstone_update(block_pos_t p) { (void)p; }
void mc_redstone_tick(tick_t t) { (void)t; }
uint8_t mc_redstone_is_power_source(block_id_t id) { (void)id; return 0; }
uint8_t mc_redstone_is_conductor(block_id_t id) { (void)id; return 0; }
uint8_t mc_redstone_is_component(block_id_t id) { (void)id; return 0; }
