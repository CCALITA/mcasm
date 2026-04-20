#include "mc_physics_internal.h"
#include <math.h>

static int aabb_overlaps_solid(aabb_t box) {
    mc_block_query_fn query = mc_physics_get_query_fn();
    if (!query) {
        return 0;
    }

    int min_x = (int)floorf(box.min.x);
    int min_y = (int)floorf(box.min.y);
    int min_z = (int)floorf(box.min.z);
    int max_x = (int)floorf(box.max.x - 0.0001f);
    int max_y = (int)floorf(box.max.y - 0.0001f);
    int max_z = (int)floorf(box.max.z - 0.0001f);

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            for (int z = min_z; z <= max_z; z++) {
                block_pos_t pos = { x, y, z, 0 };
                if (query(pos) != 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*
 * Sweep a single axis: try full displacement first; if blocked, step
 * toward the target in 1-block increments until contact.
 * min_ptr/max_ptr point to the axis component within the AABB.
 * Returns the actual displacement along that axis.
 */
static float sweep_axis(aabb_t *box, float disp, int axis) {
    if (fabsf(disp) < 0.0001f) {
        return 0.0f;
    }

    float *min_ptr = &box->min.x + axis;
    float *max_ptr = &box->max.x + axis;

    /* Try full displacement */
    aabb_t test = *box;
    float *t_min = &test.min.x + axis;
    float *t_max = &test.max.x + axis;
    *t_min += disp;
    *t_max += disp;

    if (!aabb_overlaps_solid(test)) {
        *min_ptr = *t_min;
        *max_ptr = *t_max;
        return disp;
    }

    /* Step toward target in 1-block increments until blocked */
    float sign = disp > 0.0f ? 1.0f : -1.0f;
    float remaining = fabsf(disp);
    float moved = 0.0f;

    while (remaining > 0.0001f) {
        float step = remaining < 1.0f ? remaining : 1.0f;
        aabb_t probe = *box;
        float *p_min = &probe.min.x + axis;
        float *p_max = &probe.max.x + axis;
        *p_min += sign * step;
        *p_max += sign * step;

        if (aabb_overlaps_solid(probe)) {
            break;
        }
        *box = probe;
        moved += sign * step;
        remaining -= step;
    }

    return moved;
}

vec3_t mc_physics_move_and_slide(aabb_t box, vec3_t velocity, float dt) {
    vec3_t result = { 0.0f, 0.0f, 0.0f, 0.0f };

    result.x = sweep_axis(&box, velocity.x * dt, 0);
    result.y = sweep_axis(&box, velocity.y * dt, 1);
    result.z = sweep_axis(&box, velocity.z * dt, 2);

    return result;
}

uint8_t mc_physics_test_aabb(aabb_t box) {
    return aabb_overlaps_solid(box) ? 1 : 0;
}
