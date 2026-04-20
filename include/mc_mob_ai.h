#ifndef MC_MOB_AI_H
#define MC_MOB_AI_H

#include "mc_types.h"
#include "mc_error.h"

#define MOB_ZOMBIE     0
#define MOB_SKELETON   1
#define MOB_CREEPER    2
#define MOB_SPIDER     3
#define MOB_ENDERMAN   4
#define MOB_PIG        5
#define MOB_COW        6
#define MOB_SHEEP      7
#define MOB_CHICKEN    8
#define MOB_VILLAGER   9
#define MOB_TYPE_COUNT 10

typedef block_id_t (*mc_ai_block_query_fn)(block_pos_t pos);

mc_error_t mc_mob_ai_init(mc_ai_block_query_fn block_query);
void       mc_mob_ai_shutdown(void);

mc_error_t mc_mob_ai_assign(entity_id_t entity, uint8_t mob_type);
void       mc_mob_ai_remove(entity_id_t entity);
void       mc_mob_ai_tick(tick_t current_tick);

uint8_t    mc_mob_ai_pathfind(vec3_t start, vec3_t end, vec3_t* out_path, uint32_t max_steps, uint32_t* out_length);

#endif /* MC_MOB_AI_H */
