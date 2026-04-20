#ifndef MC_CRAFTING_H
#define MC_CRAFTING_H

#include "mc_types.h"
#include "mc_error.h"

typedef struct {
    block_id_t grid[9];
    uint8_t    width;
    uint8_t    height;
    block_id_t result;
    uint8_t    result_count;
    uint8_t    shapeless;
} mc_recipe_t;

mc_error_t  mc_crafting_init(void);
void        mc_crafting_shutdown(void);
mc_error_t  mc_crafting_register(const mc_recipe_t* recipe);
block_id_t  mc_crafting_match(const block_id_t grid[9], uint8_t* out_count);
uint32_t    mc_crafting_recipe_count(void);
const mc_recipe_t* mc_crafting_get_recipe(uint32_t index);

typedef struct {
    block_id_t input;
    block_id_t output;
    float      duration;
    float      fuel_value;
} mc_smelt_recipe_t;

mc_error_t  mc_crafting_register_smelt(const mc_smelt_recipe_t* recipe);
block_id_t  mc_crafting_smelt(block_id_t input, float* out_duration);
float       mc_crafting_fuel_value(block_id_t item);

#endif /* MC_CRAFTING_H */
