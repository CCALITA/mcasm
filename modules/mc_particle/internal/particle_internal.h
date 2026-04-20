#ifndef MC_PARTICLE_INTERNAL_H
#define MC_PARTICLE_INTERNAL_H

#include "mc_types.h"
#include <stdint.h>

#define MAX_PARTICLES 4096

/*
 * SoA (Structure-of-Arrays) layout for SIMD-friendly particle update.
 * Each array is 16-byte aligned so SSE loads work directly.
 */
typedef struct {
    float pos_x[MAX_PARTICLES] __attribute__((aligned(16)));
    float pos_y[MAX_PARTICLES] __attribute__((aligned(16)));
    float pos_z[MAX_PARTICLES] __attribute__((aligned(16)));

    float vel_x[MAX_PARTICLES] __attribute__((aligned(16)));
    float vel_y[MAX_PARTICLES] __attribute__((aligned(16)));
    float vel_z[MAX_PARTICLES] __attribute__((aligned(16)));

    float lifetime[MAX_PARTICLES]       __attribute__((aligned(16)));
    float remaining_life[MAX_PARTICLES] __attribute__((aligned(16)));

    uint8_t type[MAX_PARTICLES];

    /* RGBA color packed per particle */
    float color_r[MAX_PARTICLES] __attribute__((aligned(16)));
    float color_g[MAX_PARTICLES] __attribute__((aligned(16)));
    float color_b[MAX_PARTICLES] __attribute__((aligned(16)));
    float color_a[MAX_PARTICLES] __attribute__((aligned(16)));

    uint32_t count;
} particle_pool_t;

/*
 * ASM entry point for the hot-path SIMD update.
 * Processes particles in batches of 4 using SSE:
 *   position += velocity * dt
 *   velocity.y -= gravity * dt
 *
 * Parameters:
 *   pool  - pointer to particle_pool_t
 *   count - number of active particles
 *   dt    - time step (seconds)
 */
void mc_particle_update_asm(particle_pool_t *pool, uint32_t count, float dt);

#endif /* MC_PARTICLE_INTERNAL_H */
