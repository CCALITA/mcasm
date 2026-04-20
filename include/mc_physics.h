#ifndef MC_PHYSICS_H
#define MC_PHYSICS_H

#include "mc_types.h"
#include "mc_error.h"

typedef block_id_t (*mc_block_query_fn)(block_pos_t pos);

mc_error_t       mc_physics_init(mc_block_query_fn query_fn);
void             mc_physics_shutdown(void);

vec3_t           mc_physics_move_and_slide(aabb_t box, vec3_t velocity, float dt);
raycast_result_t mc_physics_raycast(vec3_t origin, vec3_t direction, float max_distance);
uint8_t          mc_physics_test_aabb(aabb_t box);
vec3_t           mc_physics_get_fluid_flow(block_pos_t pos);
void             mc_physics_tick(float dt);

#endif /* MC_PHYSICS_H */
