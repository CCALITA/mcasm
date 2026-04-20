#include "mc_physics.h"
static mc_block_query_fn query = NULL;
mc_error_t mc_physics_init(mc_block_query_fn q) { query=q; return MC_OK; }
void mc_physics_shutdown(void) {}
vec3_t mc_physics_move_and_slide(aabb_t box, vec3_t vel, float dt) { (void)box; return (vec3_t){vel.x*dt,vel.y*dt,vel.z*dt}; }
raycast_result_t mc_physics_raycast(vec3_t o, vec3_t d, float max) { (void)o;(void)d;(void)max; return (raycast_result_t){0}; }
uint8_t mc_physics_test_aabb(aabb_t box) { (void)box; return 0; }
vec3_t mc_physics_get_fluid_flow(block_pos_t pos) { (void)pos; return (vec3_t){0}; }
void mc_physics_tick(float dt) { (void)dt; }
