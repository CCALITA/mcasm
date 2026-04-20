#include "mc_save.h"
mc_error_t mc_save_init(const char* d) { (void)d; return MC_OK; }
void mc_save_shutdown(void) {}
mc_error_t mc_save_chunk(chunk_pos_t p, const chunk_t* c) { (void)p;(void)c; return MC_OK; }
mc_error_t mc_save_load_chunk(chunk_pos_t p, chunk_t* c) { (void)p;(void)c; return MC_ERR_NOT_FOUND; }
uint8_t mc_save_chunk_exists(chunk_pos_t p) { (void)p; return 0; }
mc_error_t mc_save_player(const char* n, vec3_t p, float y, float pi) { (void)n;(void)p;(void)y;(void)pi; return MC_OK; }
mc_error_t mc_save_load_player(const char* n, vec3_t* p, float* y, float* pi) { (void)n;(void)p;(void)y;(void)pi; return MC_ERR_NOT_FOUND; }
mc_error_t mc_save_world_meta(uint32_t s, tick_t t, uint8_t gm) { (void)s;(void)t;(void)gm; return MC_OK; }
mc_error_t mc_save_load_world_meta(uint32_t* s, tick_t* t, uint8_t* gm) { (void)s;(void)t;(void)gm; return MC_ERR_NOT_FOUND; }
