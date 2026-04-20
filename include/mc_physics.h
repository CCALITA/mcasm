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

void             mc_physics_apply_gravity(vec3_t *velocity, float dt);

#define MC_PHYSICS_MAX_BODIES 256

typedef struct {
    entity_id_t entity_id;
    aabb_t      box;
    vec3_t      velocity;
    uint8_t     active;
} mc_physics_body_t;

int              mc_physics_add_body(entity_id_t id, aabb_t box, vec3_t velocity);
mc_physics_body_t *mc_physics_get_body(int index);
int              mc_physics_body_count(void);
void             mc_physics_clear_bodies(void);
void             mc_physics_tick(float dt);

#endif /* MC_PHYSICS_H */
