#ifndef MC_REDSTONE_H
#define MC_REDSTONE_H

#include "mc_types.h"
#include "mc_error.h"

typedef block_id_t (*mc_redstone_block_query_fn)(block_pos_t pos);
typedef mc_error_t (*mc_redstone_block_set_fn)(block_pos_t pos, block_id_t block);

mc_error_t mc_redstone_init(mc_redstone_block_query_fn query, mc_redstone_block_set_fn set);
void       mc_redstone_shutdown(void);

uint8_t    mc_redstone_get_power(block_pos_t pos);
void       mc_redstone_update(block_pos_t pos);
void       mc_redstone_tick(tick_t current_tick);

uint8_t    mc_redstone_is_power_source(block_id_t id);
uint8_t    mc_redstone_is_conductor(block_id_t id);
uint8_t    mc_redstone_is_component(block_id_t id);

#endif /* MC_REDSTONE_H */
