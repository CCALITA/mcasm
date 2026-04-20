#ifndef MC_ENTITY_H
#define MC_ENTITY_H

#include "mc_types.h"
#include "mc_error.h"

#define COMPONENT_TRANSFORM  (1 << 0)
#define COMPONENT_PHYSICS    (1 << 1)
#define COMPONENT_RENDER     (1 << 2)
#define COMPONENT_HEALTH     (1 << 3)
#define COMPONENT_AI         (1 << 4)
#define COMPONENT_INVENTORY  (1 << 5)
#define COMPONENT_PLAYER     (1 << 6)

#define ENTITY_INVALID 0
#define MAX_ENTITIES   4096

mc_error_t  mc_entity_init(void);
void        mc_entity_shutdown(void);

entity_id_t mc_entity_create(uint32_t component_mask);
void        mc_entity_destroy(entity_id_t id);
uint8_t     mc_entity_alive(entity_id_t id);
uint32_t    mc_entity_get_mask(entity_id_t id);
uint32_t    mc_entity_count(void);

transform_component_t* mc_entity_get_transform(entity_id_t id);
physics_component_t*   mc_entity_get_physics(entity_id_t id);
health_component_t*    mc_entity_get_health(entity_id_t id);

void mc_entity_set_position(entity_id_t id, vec3_t pos);
void mc_entity_set_velocity(entity_id_t id, vec3_t vel);

typedef void (*mc_entity_system_fn)(entity_id_t id, void* userdata);
void mc_entity_foreach(uint32_t required_mask, mc_entity_system_fn fn, void* userdata);

#endif /* MC_ENTITY_H */
