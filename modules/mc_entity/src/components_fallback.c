/*
 * components_fallback.c -- Pure-C fallback for components.asm
 *
 * Used on non-x86-64 architectures (e.g. Apple Silicon) where NASM
 * cannot produce native code.  Provides component accessors that
 * reference the SoA arrays defined in ecs_fallback.c.
 */

#include "mc_entity.h"

#ifndef __x86_64__

/* SoA arrays defined in ecs_fallback.c */
extern transform_component_t ecs_transforms[];
extern physics_component_t   ecs_physics[];
extern health_component_t    ecs_healths[];

transform_component_t *mc_entity_get_transform(entity_id_t id)
{
    if (id >= MAX_ENTITIES) return NULL;
    return &ecs_transforms[id];
}

physics_component_t *mc_entity_get_physics(entity_id_t id)
{
    if (id >= MAX_ENTITIES) return NULL;
    return &ecs_physics[id];
}

health_component_t *mc_entity_get_health(entity_id_t id)
{
    if (id >= MAX_ENTITIES) return NULL;
    return &ecs_healths[id];
}

void mc_entity_set_position(entity_id_t id, vec3_t pos)
{
    if (id >= MAX_ENTITIES) return;
    ecs_transforms[id].position = pos;
}

void mc_entity_set_velocity(entity_id_t id, vec3_t vel)
{
    if (id >= MAX_ENTITIES) return;
    ecs_transforms[id].velocity = vel;
}

#endif /* !__x86_64__ */
