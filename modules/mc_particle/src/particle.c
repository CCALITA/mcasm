#include "mc_particle.h"
#include "particle_internal.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/*  Module-private state                                               */
/* ------------------------------------------------------------------ */

/* Not static: shared with render.c within this module. */
particle_pool_t g_pool;
static int      g_initialized;

/* Default color table indexed by particle type (RGBA). */
static const float g_type_colors[PARTICLE_TYPE_COUNT][4] = {
    [PARTICLE_BLOCK_BREAK] = { 0.6f, 0.4f, 0.2f, 1.0f },
    [PARTICLE_FLAME]       = { 1.0f, 0.5f, 0.0f, 1.0f },
    [PARTICLE_SMOKE]       = { 0.3f, 0.3f, 0.3f, 0.8f },
    [PARTICLE_RAIN]        = { 0.4f, 0.5f, 0.9f, 0.6f },
    [PARTICLE_SPLASH]      = { 0.5f, 0.7f, 1.0f, 0.7f },
    [PARTICLE_DUST]        = { 0.7f, 0.6f, 0.5f, 0.5f },
    [PARTICLE_BUBBLE]      = { 0.8f, 0.9f, 1.0f, 0.4f },
    [PARTICLE_EXPLOSION]   = { 1.0f, 0.3f, 0.1f, 1.0f },
};

/* ------------------------------------------------------------------ */
/*  Init / Shutdown                                                    */
/* ------------------------------------------------------------------ */

mc_error_t mc_particle_init(void)
{
    memset(&g_pool, 0, sizeof(g_pool));
    g_initialized = 1;
    return MC_OK;
}

void mc_particle_shutdown(void)
{
    memset(&g_pool, 0, sizeof(g_pool));
    g_initialized = 0;
}

/* ------------------------------------------------------------------ */
/*  Emit                                                               */
/* ------------------------------------------------------------------ */

void mc_particle_emit(uint8_t type, vec3_t position, vec3_t velocity,
                      float lifetime, uint32_t count)
{
    if (!g_initialized) return;
    if (type >= PARTICLE_TYPE_COUNT) return;
    if (lifetime <= 0.0f) return;

    for (uint32_t i = 0; i < count; i++) {
        if (g_pool.count >= MAX_PARTICLES) return;

        uint32_t idx = g_pool.count;

        g_pool.pos_x[idx] = position.x;
        g_pool.pos_y[idx] = position.y;
        g_pool.pos_z[idx] = position.z;

        g_pool.vel_x[idx] = velocity.x;
        g_pool.vel_y[idx] = velocity.y;
        g_pool.vel_z[idx] = velocity.z;

        g_pool.lifetime[idx]       = lifetime;
        g_pool.remaining_life[idx] = lifetime;

        g_pool.type[idx] = type;

        g_pool.color_r[idx] = g_type_colors[type][0];
        g_pool.color_g[idx] = g_type_colors[type][1];
        g_pool.color_b[idx] = g_type_colors[type][2];
        g_pool.color_a[idx] = g_type_colors[type][3];

        g_pool.count++;
    }
}

/* ------------------------------------------------------------------ */
/*  Tick — update particles, cull dead ones                            */
/* ------------------------------------------------------------------ */

/* Swap-remove: overwrite slot idx with the last alive particle, then shrink. */
static void remove_particle(uint32_t idx)
{
    uint32_t last = g_pool.count - 1;

    if (idx != last) {
        g_pool.pos_x[idx] = g_pool.pos_x[last];
        g_pool.pos_y[idx] = g_pool.pos_y[last];
        g_pool.pos_z[idx] = g_pool.pos_z[last];

        g_pool.vel_x[idx] = g_pool.vel_x[last];
        g_pool.vel_y[idx] = g_pool.vel_y[last];
        g_pool.vel_z[idx] = g_pool.vel_z[last];

        g_pool.lifetime[idx]       = g_pool.lifetime[last];
        g_pool.remaining_life[idx] = g_pool.remaining_life[last];

        g_pool.type[idx] = g_pool.type[last];

        g_pool.color_r[idx] = g_pool.color_r[last];
        g_pool.color_g[idx] = g_pool.color_g[last];
        g_pool.color_b[idx] = g_pool.color_b[last];
        g_pool.color_a[idx] = g_pool.color_a[last];
    }

    g_pool.count--;
}

void mc_particle_tick(float dt)
{
    if (!g_initialized) return;
    if (dt <= 0.0f) return;

    /* 1. Physics update via SIMD ASM (position += vel*dt, gravity) */
    if (g_pool.count > 0) {
        mc_particle_update_asm(&g_pool, g_pool.count, dt);
    }

    /* 2. Decrement remaining_life and remove dead particles */
    uint32_t i = 0;
    while (i < g_pool.count) {
        g_pool.remaining_life[i] -= dt;
        if (g_pool.remaining_life[i] <= 0.0f) {
            remove_particle(i);
            /* Don't increment — the swapped-in particle needs checking */
        } else {
            i++;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Count                                                              */
/* ------------------------------------------------------------------ */

uint32_t mc_particle_count(void)
{
    return g_pool.count;
}
