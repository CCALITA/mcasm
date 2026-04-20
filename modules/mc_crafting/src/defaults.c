#include "crafting_internal.h"

/* ------------------------------------------------------------------ */
/*  Helper to build a recipe struct                                    */
/* ------------------------------------------------------------------ */

static mc_recipe_t make_recipe(uint8_t w, uint8_t h,
                               const block_id_t *pattern,
                               block_id_t result, uint8_t count,
                               uint8_t shapeless) {
    mc_recipe_t r = {
        .width        = w,
        .height       = h,
        .result       = result,
        .result_count = count,
        .shapeless    = shapeless,
        .grid         = {0}
    };
    for (int i = 0; i < w * h; i++) {
        r.grid[i] = pattern[i];
    }
    return r;
}

/* ------------------------------------------------------------------ */
/*  Default crafting recipes                                           */
/* ------------------------------------------------------------------ */

static void register_crafting_defaults(void) {
    /* Planks from log — shapeless, yields 4. */
    {
        const block_id_t pat[] = { BLOCK_OAK_LOG };
        mc_recipe_t r = make_recipe(1, 1, pat, BLOCK_OAK_PLANKS, 4, 1);
        mc_crafting_register(&r);
    }

    /* Crafting table from 4 planks (2x2). */
    {
        const block_id_t pat[] = {
            BLOCK_OAK_PLANKS, BLOCK_OAK_PLANKS,
            BLOCK_OAK_PLANKS, BLOCK_OAK_PLANKS
        };
        mc_recipe_t r = make_recipe(2, 2, pat, BLOCK_CRAFTING_TABLE, 1, 0);
        mc_crafting_register(&r);
    }

    /* Sticks from 2 planks (vertical 1x2). */
    {
        const block_id_t pat[] = {
            BLOCK_OAK_PLANKS,
            BLOCK_OAK_PLANKS
        };
        mc_recipe_t r = make_recipe(1, 2, pat, ITEM_STICK, 4, 0);
        mc_crafting_register(&r);
    }

    /* Wooden pickaxe (3x3 with pattern). */
    {
        const block_id_t pat[] = {
            BLOCK_OAK_PLANKS, BLOCK_OAK_PLANKS, BLOCK_OAK_PLANKS,
            BLOCK_AIR,        ITEM_STICK,        BLOCK_AIR,
            BLOCK_AIR,        ITEM_STICK,        BLOCK_AIR
        };
        mc_recipe_t r = make_recipe(3, 3, pat, ITEM_WOODEN_PICKAXE, 1, 0);
        mc_crafting_register(&r);
    }

    /* Stone pickaxe. */
    {
        const block_id_t pat[] = {
            BLOCK_COBBLESTONE, BLOCK_COBBLESTONE, BLOCK_COBBLESTONE,
            BLOCK_AIR,         ITEM_STICK,        BLOCK_AIR,
            BLOCK_AIR,         ITEM_STICK,        BLOCK_AIR
        };
        mc_recipe_t r = make_recipe(3, 3, pat, ITEM_STONE_PICKAXE, 1, 0);
        mc_crafting_register(&r);
    }

    /* Iron pickaxe. */
    {
        const block_id_t pat[] = {
            ITEM_IRON_INGOT, ITEM_IRON_INGOT, ITEM_IRON_INGOT,
            BLOCK_AIR,       ITEM_STICK,      BLOCK_AIR,
            BLOCK_AIR,       ITEM_STICK,      BLOCK_AIR
        };
        mc_recipe_t r = make_recipe(3, 3, pat, ITEM_IRON_PICKAXE, 1, 0);
        mc_crafting_register(&r);
    }

    /* Furnace from 8 cobblestone (3x3 ring). */
    {
        const block_id_t pat[] = {
            BLOCK_COBBLESTONE, BLOCK_COBBLESTONE, BLOCK_COBBLESTONE,
            BLOCK_COBBLESTONE, BLOCK_AIR,         BLOCK_COBBLESTONE,
            BLOCK_COBBLESTONE, BLOCK_COBBLESTONE, BLOCK_COBBLESTONE
        };
        mc_recipe_t r = make_recipe(3, 3, pat, BLOCK_FURNACE, 1, 0);
        mc_crafting_register(&r);
    }

    /* Torch from stick + coal (shapeless). */
    {
        const block_id_t pat[] = {
            ITEM_COAL,
            ITEM_STICK
        };
        mc_recipe_t r = make_recipe(1, 2, pat, BLOCK_TORCH, 4, 1);
        mc_crafting_register(&r);
    }

    /* Chest from 8 planks (3x3 ring). */
    {
        const block_id_t pat[] = {
            BLOCK_OAK_PLANKS, BLOCK_OAK_PLANKS, BLOCK_OAK_PLANKS,
            BLOCK_OAK_PLANKS, BLOCK_AIR,        BLOCK_OAK_PLANKS,
            BLOCK_OAK_PLANKS, BLOCK_OAK_PLANKS, BLOCK_OAK_PLANKS
        };
        mc_recipe_t r = make_recipe(3, 3, pat, BLOCK_CHEST, 1, 0);
        mc_crafting_register(&r);
    }
}

/* ------------------------------------------------------------------ */
/*  Default smelting recipes                                           */
/* ------------------------------------------------------------------ */

static void register_smelting_defaults(void) {
    /* Iron ore -> iron ingot, 10s */
    {
        mc_smelt_recipe_t s = {
            .input      = BLOCK_IRON_ORE,
            .output     = ITEM_IRON_INGOT,
            .duration   = 10.0f,
            .fuel_value = 0.0f
        };
        mc_crafting_register_smelt(&s);
    }

    /* Cobblestone -> stone, 10s */
    {
        mc_smelt_recipe_t s = {
            .input      = BLOCK_COBBLESTONE,
            .output     = BLOCK_STONE,
            .duration   = 10.0f,
            .fuel_value = 0.0f
        };
        mc_crafting_register_smelt(&s);
    }

    /* Sand -> glass, 10s */
    {
        mc_smelt_recipe_t s = {
            .input      = BLOCK_SAND,
            .output     = BLOCK_GLASS,
            .duration   = 10.0f,
            .fuel_value = 0.0f
        };
        mc_crafting_register_smelt(&s);
    }

    /* Fuel entries — stored as smelt recipes with output == BLOCK_AIR. */

    /* Coal burns for 80s */
    {
        mc_smelt_recipe_t s = {
            .input      = ITEM_COAL,
            .output     = BLOCK_AIR,
            .duration   = 0.0f,
            .fuel_value = 80.0f
        };
        mc_crafting_register_smelt(&s);
    }

    /* Oak log burns for 15s */
    {
        mc_smelt_recipe_t s = {
            .input      = BLOCK_OAK_LOG,
            .output     = BLOCK_AIR,
            .duration   = 0.0f,
            .fuel_value = 15.0f
        };
        mc_crafting_register_smelt(&s);
    }

    /* Oak planks burn for 15s */
    {
        mc_smelt_recipe_t s = {
            .input      = BLOCK_OAK_PLANKS,
            .output     = BLOCK_AIR,
            .duration   = 0.0f,
            .fuel_value = 15.0f
        };
        mc_crafting_register_smelt(&s);
    }

    /* Stick burns for 5s */
    {
        mc_smelt_recipe_t s = {
            .input      = ITEM_STICK,
            .output     = BLOCK_AIR,
            .duration   = 0.0f,
            .fuel_value = 5.0f
        };
        mc_crafting_register_smelt(&s);
    }
}

/* ------------------------------------------------------------------ */
/*  Public entry point                                                 */
/* ------------------------------------------------------------------ */

void mc_crafting_register_defaults(void) {
    register_crafting_defaults();
    register_smelting_defaults();
}
