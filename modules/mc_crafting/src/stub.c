#include "mc_crafting.h"
mc_error_t mc_crafting_init(void) { return MC_OK; }
void mc_crafting_shutdown(void) {}
mc_error_t mc_crafting_register(const mc_recipe_t* r) { (void)r; return MC_OK; }
block_id_t mc_crafting_match(const block_id_t grid[9], uint8_t* c) { (void)grid; *c=0; return 0; }
uint32_t mc_crafting_recipe_count(void) { return 0; }
const mc_recipe_t* mc_crafting_get_recipe(uint32_t i) { (void)i; return NULL; }
mc_error_t mc_crafting_register_smelt(const mc_smelt_recipe_t* r) { (void)r; return MC_OK; }
block_id_t mc_crafting_smelt(block_id_t i, float* d) { (void)i; *d=0; return 0; }
float mc_crafting_fuel_value(block_id_t i) { (void)i; return 0; }
