#ifndef MC_PARTICLE_H
#define MC_PARTICLE_H

#include "mc_types.h"
#include "mc_error.h"

#define PARTICLE_BLOCK_BREAK  0
#define PARTICLE_FLAME        1
#define PARTICLE_SMOKE         2
#define PARTICLE_RAIN         3
#define PARTICLE_SPLASH       4
#define PARTICLE_DUST         5
#define PARTICLE_BUBBLE       6
#define PARTICLE_EXPLOSION    7
#define PARTICLE_TYPE_COUNT   8

mc_error_t mc_particle_init(void);
void       mc_particle_shutdown(void);

void       mc_particle_emit(uint8_t type, vec3_t position, vec3_t velocity, float lifetime, uint32_t count);
void       mc_particle_tick(float dt);
void       mc_particle_render(const mat4_t* view, const mat4_t* projection);
uint32_t   mc_particle_count(void);

#endif /* MC_PARTICLE_H */
