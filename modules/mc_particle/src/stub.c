#include "mc_particle.h"
mc_error_t mc_particle_init(void) { return MC_OK; }
void mc_particle_shutdown(void) {}
void mc_particle_emit(uint8_t t, vec3_t p, vec3_t v, float l, uint32_t c) { (void)t;(void)p;(void)v;(void)l;(void)c; }
void mc_particle_tick(float dt) { (void)dt; }
void mc_particle_render(const mat4_t* v, const mat4_t* p) { (void)v;(void)p; }
uint32_t mc_particle_count(void) { return 0; }
