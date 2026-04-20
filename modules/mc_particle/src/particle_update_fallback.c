/*
 * particle_update_fallback.c — Pure-C fallback for mc_particle_update_asm.
 *
 * Used on non-x86-64 architectures (e.g. Apple Silicon) where NASM
 * cannot produce native code.  Implements the same logic as
 * particle_update.asm: position += velocity * dt, gravity on Y axis.
 */
#include "particle_internal.h"

#define GRAVITY 9.81f

void mc_particle_update_asm(particle_pool_t *pool, uint32_t count, float dt)
{
    for (uint32_t i = 0; i < count; i++) {
        /* Apply gravity to vertical velocity */
        pool->vel_y[i] -= GRAVITY * dt;

        /* Integrate position */
        pool->pos_x[i] += pool->vel_x[i] * dt;
        pool->pos_y[i] += pool->vel_y[i] * dt;
        pool->pos_z[i] += pool->vel_z[i] * dt;
    }
}
