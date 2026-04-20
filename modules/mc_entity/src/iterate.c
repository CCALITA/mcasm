/*
 * iterate.c — Entity iteration with mask filtering.
 *
 * Implemented in C for clarity; the hot inner loop is a simple
 * bitmask check that the compiler vectorises well.
 */

#include "mc_entity.h"

/* SoA arrays and scalars exported from ecs.asm */
extern uint32_t ecs_masks[];
extern uint32_t ecs_next_id;

void mc_entity_foreach(uint32_t required_mask,
                       mc_entity_system_fn fn,
                       void *userdata)
{
    const uint32_t high_id = ecs_next_id;

    for (uint32_t i = 1; i < high_id; ++i) {
        if ((ecs_masks[i] & required_mask) == required_mask) {
            fn(i, userdata);
        }
    }
}
