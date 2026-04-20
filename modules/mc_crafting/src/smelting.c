#include "crafting_internal.h"

/* ------------------------------------------------------------------ */
/*  Smelting registry                                                  */
/* ------------------------------------------------------------------ */

mc_smelt_recipe_t g_smelt_recipes[MAX_SMELT_RECIPES];
uint32_t          g_smelt_count = 0;

/* ------------------------------------------------------------------ */
/*  Registration                                                       */
/* ------------------------------------------------------------------ */

mc_error_t mc_crafting_register_smelt(const mc_smelt_recipe_t *recipe) {
    if (!recipe) {
        return MC_ERR_INVALID_ARG;
    }
    if (g_smelt_count >= MAX_SMELT_RECIPES) {
        return MC_ERR_FULL;
    }
    g_smelt_recipes[g_smelt_count] = *recipe;
    g_smelt_count++;
    return MC_OK;
}

/* ------------------------------------------------------------------ */
/*  Lookup                                                             */
/* ------------------------------------------------------------------ */

block_id_t mc_crafting_smelt(block_id_t input, float *out_duration) {
    if (!out_duration) {
        return BLOCK_AIR;
    }
    for (uint32_t i = 0; i < g_smelt_count; i++) {
        if (g_smelt_recipes[i].input == input &&
            g_smelt_recipes[i].output != BLOCK_AIR) {
            *out_duration = g_smelt_recipes[i].duration;
            return g_smelt_recipes[i].output;
        }
    }
    *out_duration = 0.0f;
    return BLOCK_AIR;
}

float mc_crafting_fuel_value(block_id_t item) {
    for (uint32_t i = 0; i < g_smelt_count; i++) {
        if (g_smelt_recipes[i].input == item && g_smelt_recipes[i].fuel_value > 0.0f) {
            return g_smelt_recipes[i].fuel_value;
        }
    }
    return 0.0f;
}
