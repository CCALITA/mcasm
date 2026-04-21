/*
 * ecs_fallback.c -- Pure-C fallback for ecs.asm
 *
 * Used on non-x86-64 architectures (e.g. Apple Silicon) where NASM
 * cannot produce native code.  Implements the same ECS lifecycle
 * logic as ecs.asm: SoA arrays, free list, entity create/destroy.
 */

#include "mc_entity.h"
#include <string.h>

#ifndef __x86_64__

/* SoA arrays — exported so iterate.c and components_fallback.c can use them */
uint32_t               ecs_masks[MAX_ENTITIES];
transform_component_t  ecs_transforms[MAX_ENTITIES];
physics_component_t    ecs_physics[MAX_ENTITIES];
health_component_t     ecs_healths[MAX_ENTITIES];

uint32_t ecs_entity_count;
uint32_t ecs_next_id;

static uint32_t free_list[MAX_ENTITIES];
static uint32_t free_top;

mc_error_t mc_entity_init(void)
{
    memset(ecs_masks, 0, sizeof(ecs_masks));
    ecs_entity_count = 0;
    ecs_next_id      = 1;
    free_top         = 0;
    return MC_OK;
}

void mc_entity_shutdown(void)
{
    /* No-op: static arrays need no deallocation. */
}

entity_id_t mc_entity_create(uint32_t component_mask)
{
    uint32_t id;

    if (free_top > 0) {
        free_top--;
        id = free_list[free_top];
    } else {
        if (ecs_next_id >= MAX_ENTITIES)
            return ENTITY_INVALID;
        id = ecs_next_id++;
    }

    ecs_masks[id] = component_mask;
    ecs_entity_count++;
    return id;
}

void mc_entity_destroy(entity_id_t id)
{
    if (id == 0 || id >= MAX_ENTITIES) return;
    if (ecs_masks[id] == 0) return;

    ecs_masks[id] = 0;
    free_list[free_top++] = id;
    ecs_entity_count--;
}

uint8_t mc_entity_alive(entity_id_t id)
{
    if (id == 0 || id >= MAX_ENTITIES) return 0;
    return ecs_masks[id] != 0 ? 1 : 0;
}

uint32_t mc_entity_get_mask(entity_id_t id)
{
    if (id >= MAX_ENTITIES) return 0;
    return ecs_masks[id];
}

uint32_t mc_entity_count(void)
{
    return ecs_entity_count;
}

#endif /* !__x86_64__ */
