#include "mc_physics.h"

#define MC_GRAVITY 20.0f
#define MC_COLLISION_RATIO_THRESHOLD 0.99f

static mc_block_query_fn g_block_query = NULL;

static mc_physics_body_t g_bodies[MC_PHYSICS_MAX_BODIES];
static int               g_body_count = 0;

mc_block_query_fn mc_physics_get_query_fn(void) {
    return g_block_query;
}

mc_error_t mc_physics_init(mc_block_query_fn query_fn) {
    if (!query_fn) {
        return MC_ERR_INVALID_ARG;
    }
    g_block_query = query_fn;
    mc_physics_clear_bodies();
    return MC_OK;
}

void mc_physics_shutdown(void) {
    g_block_query = NULL;
    mc_physics_clear_bodies();
}

void mc_physics_apply_gravity(vec3_t *velocity, float dt) {
    if (!velocity) {
        return;
    }
    velocity->y -= MC_GRAVITY * dt;
}

int mc_physics_add_body(entity_id_t id, aabb_t box, vec3_t velocity) {
    if (g_body_count >= MC_PHYSICS_MAX_BODIES) {
        return -1;
    }
    int index = g_body_count;
    g_bodies[index].entity_id = id;
    g_bodies[index].box = box;
    g_bodies[index].velocity = velocity;
    g_bodies[index].active = 1;
    g_body_count++;
    return index;
}

mc_physics_body_t *mc_physics_get_body(int index) {
    if (index < 0 || index >= g_body_count) {
        return NULL;
    }
    return &g_bodies[index];
}

int mc_physics_body_count(void) {
    return g_body_count;
}

void mc_physics_clear_bodies(void) {
    g_body_count = 0;
    for (int i = 0; i < MC_PHYSICS_MAX_BODIES; i++) {
        g_bodies[i].active = 0;
    }
}

void mc_physics_tick(float dt) {
    if (dt <= 0.0f || !g_block_query) {
        return;
    }

    for (int i = 0; i < g_body_count; i++) {
        if (!g_bodies[i].active) {
            continue;
        }

        mc_physics_apply_gravity(&g_bodies[i].velocity, dt);

        vec3_t disp = mc_physics_move_and_slide(
            g_bodies[i].box,
            g_bodies[i].velocity,
            dt
        );

        g_bodies[i].box.min.x += disp.x;
        g_bodies[i].box.max.x += disp.x;
        g_bodies[i].box.min.y += disp.y;
        g_bodies[i].box.max.y += disp.y;
        g_bodies[i].box.min.z += disp.z;
        g_bodies[i].box.max.z += disp.z;

        /* If collision stopped vertical movement, zero vertical velocity */
        float expected_y = g_bodies[i].velocity.y * dt;
        if (expected_y != 0.0f) {
            float ratio = disp.y / expected_y;
            if (ratio < MC_COLLISION_RATIO_THRESHOLD) {
                g_bodies[i].velocity.y = 0.0f;
            }
        }
    }
}
