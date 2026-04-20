#include "mc_particle.h"
#include "particle_internal.h"

/*
 * Instance data for GPU instanced rendering.
 * In a full engine this would write into a mapped Vulkan staging buffer.
 * For now we prepare into a module-local array for the render pass to upload.
 */

typedef struct {
    float pos[3];
    float color[4];
    float size;
} particle_instance_t;

#define MAX_INSTANCES MAX_PARTICLES
static particle_instance_t g_instances[MAX_INSTANCES];
static uint32_t            g_instance_count;

extern particle_pool_t g_pool;

static float clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

void mc_particle_render(const mat4_t *view, const mat4_t *projection)
{
    (void)view;
    (void)projection;

    g_instance_count = g_pool.count;

    for (uint32_t i = 0; i < g_pool.count; i++) {
        particle_instance_t *inst = &g_instances[i];

        inst->pos[0] = g_pool.pos_x[i];
        inst->pos[1] = g_pool.pos_y[i];
        inst->pos[2] = g_pool.pos_z[i];

        float life_ratio = (g_pool.lifetime[i] > 0.0f)
            ? clamp01(g_pool.remaining_life[i] / g_pool.lifetime[i])
            : 0.0f;

        inst->color[0] = g_pool.color_r[i];
        inst->color[1] = g_pool.color_g[i];
        inst->color[2] = g_pool.color_b[i];
        inst->color[3] = g_pool.color_a[i] * life_ratio;

        inst->size = 0.1f + 0.15f * life_ratio;
    }

    /*
     * At this point g_instances[0..g_instance_count-1] contains the
     * instance data ready for upload to a Vulkan vertex buffer and
     * drawing via vkCmdDrawIndexed with instancing.
     *
     * The actual Vulkan calls belong in mc_render; this module just
     * prepares the data.
     */
}
