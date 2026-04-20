#ifndef CRAFTING_INTERNAL_H
#define CRAFTING_INTERNAL_H

#include "mc_crafting.h"
#include "mc_block.h"

/* Item IDs start above the block range (BLOCK_TYPE_COUNT = 256). */
#define ITEM_STICK             256
#define ITEM_COAL              257
#define ITEM_IRON_INGOT        258
#define ITEM_GOLD_INGOT        259
#define ITEM_DIAMOND           260
#define ITEM_WOODEN_PICKAXE    261
#define ITEM_STONE_PICKAXE     262
#define ITEM_IRON_PICKAXE      263

#define MAX_RECIPES       256
#define MAX_SMELT_RECIPES 64

/* Shared registry state — owned by recipes.c */
extern mc_recipe_t       g_recipes[];
extern uint32_t          g_recipe_count;

/* Shared smelting state — owned by smelting.c */
extern mc_smelt_recipe_t g_smelt_recipes[];
extern uint32_t          g_smelt_count;

/* Called by mc_crafting_init to register built-in recipes. */
void mc_crafting_register_defaults(void);

#endif /* CRAFTING_INTERNAL_H */
