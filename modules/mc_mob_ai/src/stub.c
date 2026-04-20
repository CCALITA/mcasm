#include "mc_mob_ai.h"
mc_error_t mc_mob_ai_init(mc_ai_block_query_fn q) { (void)q; return MC_OK; }
void mc_mob_ai_shutdown(void) {}
mc_error_t mc_mob_ai_assign(entity_id_t e, uint8_t t) { (void)e;(void)t; return MC_OK; }
void mc_mob_ai_remove(entity_id_t e) { (void)e; }
void mc_mob_ai_tick(tick_t t) { (void)t; }
uint8_t mc_mob_ai_pathfind(vec3_t s, vec3_t e, vec3_t* p, uint32_t mx, uint32_t* ol) { (void)s;(void)e;(void)p;(void)mx; *ol=0; return 0; }
