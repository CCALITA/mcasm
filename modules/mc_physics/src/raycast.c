#include "mc_physics_internal.h"
#include <math.h>

raycast_result_t mc_physics_raycast(vec3_t origin, vec3_t direction, float max_distance) {
    raycast_result_t result = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, 0.0f, 0 };
    mc_block_query_fn query = mc_physics_get_query_fn();

    if (!query) {
        return result;
    }

    /* Normalize direction */
    float len = sqrtf(direction.x * direction.x +
                      direction.y * direction.y +
                      direction.z * direction.z);
    if (len < 0.000001f) {
        return result;
    }
    float dx = direction.x / len;
    float dy = direction.y / len;
    float dz = direction.z / len;

    /* Current block position */
    int bx = (int)floorf(origin.x);
    int by = (int)floorf(origin.y);
    int bz = (int)floorf(origin.z);

    /* Step direction (+1 or -1) */
    int step_x = dx >= 0.0f ? 1 : -1;
    int step_y = dy >= 0.0f ? 1 : -1;
    int step_z = dz >= 0.0f ? 1 : -1;

    /* Distance along ray per unit step in each axis */
    float t_delta_x = (fabsf(dx) > 0.000001f) ? fabsf(1.0f / dx) : 1e30f;
    float t_delta_y = (fabsf(dy) > 0.000001f) ? fabsf(1.0f / dy) : 1e30f;
    float t_delta_z = (fabsf(dz) > 0.000001f) ? fabsf(1.0f / dz) : 1e30f;

    /* Distance to next block boundary in each axis */
    float t_max_x, t_max_y, t_max_z;
    if (dx >= 0.0f) {
        t_max_x = ((float)(bx + 1) - origin.x) * t_delta_x;
    } else {
        t_max_x = (origin.x - (float)bx) * t_delta_x;
    }
    if (dy >= 0.0f) {
        t_max_y = ((float)(by + 1) - origin.y) * t_delta_y;
    } else {
        t_max_y = (origin.y - (float)by) * t_delta_y;
    }
    if (dz >= 0.0f) {
        t_max_z = ((float)(bz + 1) - origin.z) * t_delta_z;
    } else {
        t_max_z = (origin.z - (float)bz) * t_delta_z;
    }

    float distance = 0.0f;

    /* DDA loop */
    while (distance <= max_distance) {
        block_pos_t pos = { bx, by, bz, 0 };
        if (query(pos) != 0) {
            result.hit = 1;
            result.distance = distance;
            result.block_pos = pos;
            result.hit_point = (vec3_t){
                origin.x + dx * distance,
                origin.y + dy * distance,
                origin.z + dz * distance,
                0.0f
            };
            return result;
        }

        /* Advance to the next block boundary along the closest axis */
        if (t_max_x < t_max_y) {
            if (t_max_x < t_max_z) {
                bx += step_x;
                distance = t_max_x;
                result.hit_normal = (vec3_t){ (float)(-step_x), 0.0f, 0.0f, 0.0f };
                t_max_x += t_delta_x;
            } else {
                bz += step_z;
                distance = t_max_z;
                result.hit_normal = (vec3_t){ 0.0f, 0.0f, (float)(-step_z), 0.0f };
                t_max_z += t_delta_z;
            }
        } else {
            if (t_max_y < t_max_z) {
                by += step_y;
                distance = t_max_y;
                result.hit_normal = (vec3_t){ 0.0f, (float)(-step_y), 0.0f, 0.0f };
                t_max_y += t_delta_y;
            } else {
                bz += step_z;
                distance = t_max_z;
                result.hit_normal = (vec3_t){ 0.0f, 0.0f, (float)(-step_z), 0.0f };
                t_max_z += t_delta_z;
            }
        }
    }

    return result;
}
