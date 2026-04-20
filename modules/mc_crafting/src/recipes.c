#include "crafting_internal.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Recipe registry                                                    */
/* ------------------------------------------------------------------ */

mc_recipe_t g_recipes[MAX_RECIPES];
uint32_t    g_recipe_count = 0;

/* ------------------------------------------------------------------ */
/*  Lifecycle                                                          */
/* ------------------------------------------------------------------ */

mc_error_t mc_crafting_init(void) {
    g_recipe_count = 0;
    g_smelt_count  = 0;
    mc_crafting_register_defaults();
    return MC_OK;
}

void mc_crafting_shutdown(void) {
    g_recipe_count = 0;
    g_smelt_count  = 0;
}

/* ------------------------------------------------------------------ */
/*  Registration                                                       */
/* ------------------------------------------------------------------ */

mc_error_t mc_crafting_register(const mc_recipe_t *recipe) {
    if (!recipe) {
        return MC_ERR_INVALID_ARG;
    }
    if (g_recipe_count >= MAX_RECIPES) {
        return MC_ERR_FULL;
    }
    g_recipes[g_recipe_count] = *recipe;
    g_recipe_count++;
    return MC_OK;
}

uint32_t mc_crafting_recipe_count(void) {
    return g_recipe_count;
}

const mc_recipe_t *mc_crafting_get_recipe(uint32_t index) {
    if (index >= g_recipe_count) {
        return NULL;
    }
    return &g_recipes[index];
}

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

/* Count non-AIR items in a 9-slot grid. */
static uint32_t count_items(const block_id_t grid[9]) {
    uint32_t n = 0;
    for (int i = 0; i < 9; i++) {
        if (grid[i] != BLOCK_AIR) {
            n++;
        }
    }
    return n;
}

/* Sort 9 item ids into buf (ascending). Returns count of non-AIR. */
static uint32_t sorted_items(const block_id_t grid[9], block_id_t buf[9]) {
    uint32_t n = 0;
    for (int i = 0; i < 9; i++) {
        if (grid[i] != BLOCK_AIR) {
            buf[n] = grid[i];
            n++;
        }
    }
    /* Simple insertion sort — at most 9 elements. */
    for (uint32_t i = 1; i < n; i++) {
        block_id_t key = buf[i];
        uint32_t j = i;
        while (j > 0 && buf[j - 1] > key) {
            buf[j] = buf[j - 1];
            j--;
        }
        buf[j] = key;
    }
    return n;
}

/* Check if recipe pattern at (row,col) offset in the 3x3 grid matches. */
static int match_shaped_at(const mc_recipe_t *recipe,
                           const block_id_t grid[9],
                           int row_off, int col_off) {
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            int gi = r * 3 + c;
            int pr = r - row_off;
            int pc = c - col_off;
            block_id_t expected = BLOCK_AIR;
            if (pr >= 0 && pr < recipe->height &&
                pc >= 0 && pc < recipe->width) {
                expected = recipe->grid[pr * recipe->width + pc];
            }
            if (grid[gi] != expected) {
                return 0;
            }
        }
    }
    return 1;
}

/* Try all valid placements of a shaped recipe inside a 3x3 grid. */
static int match_shaped(const mc_recipe_t *recipe,
                        const block_id_t grid[9]) {
    int max_row = 3 - recipe->height;
    int max_col = 3 - recipe->width;
    for (int r = 0; r <= max_row; r++) {
        for (int c = 0; c <= max_col; c++) {
            if (match_shaped_at(recipe, grid, r, c)) {
                return 1;
            }
        }
    }
    return 0;
}

/* Build a rotated recipe (90 degrees clockwise). */
static mc_recipe_t rotate_90(const mc_recipe_t *src) {
    mc_recipe_t dst;
    memset(&dst, 0, sizeof(dst));
    dst.width        = src->height;
    dst.height       = src->width;
    dst.result       = src->result;
    dst.result_count = src->result_count;
    dst.shapeless    = src->shapeless;
    for (int r = 0; r < src->height; r++) {
        for (int c = 0; c < src->width; c++) {
            int nr = c;
            int nc = (src->height - 1) - r;
            dst.grid[nr * dst.width + nc] = src->grid[r * src->width + c];
        }
    }
    return dst;
}

/* Build a horizontally flipped recipe. */
static mc_recipe_t flip_h(const mc_recipe_t *src) {
    mc_recipe_t dst;
    memset(&dst, 0, sizeof(dst));
    dst.width        = src->width;
    dst.height       = src->height;
    dst.result       = src->result;
    dst.result_count = src->result_count;
    dst.shapeless    = src->shapeless;
    for (int r = 0; r < src->height; r++) {
        for (int c = 0; c < src->width; c++) {
            int nc = (src->width - 1) - c;
            dst.grid[r * dst.width + nc] = src->grid[r * src->width + c];
        }
    }
    return dst;
}

/* Try a shaped recipe with all 4 rotations x 2 flips. */
static int match_shaped_all_orientations(const mc_recipe_t *recipe,
                                         const block_id_t grid[9]) {
    mc_recipe_t rot = *recipe;
    for (int f = 0; f < 2; f++) {
        for (int r = 0; r < 4; r++) {
            if (match_shaped(&rot, grid)) {
                return 1;
            }
            rot = rotate_90(&rot);
        }
        rot = flip_h(recipe);
    }
    return 0;
}

/* Match a shapeless recipe: same items regardless of position. */
static int match_shapeless(const mc_recipe_t *recipe,
                           const block_id_t grid[9]) {
    block_id_t recipe_sorted[9];
    block_id_t grid_sorted[9];

    uint32_t rn = sorted_items(recipe->grid, recipe_sorted);
    uint32_t gn = sorted_items(grid, grid_sorted);

    if (rn != gn) {
        return 0;
    }
    for (uint32_t i = 0; i < rn; i++) {
        if (recipe_sorted[i] != grid_sorted[i]) {
            return 0;
        }
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Public matching                                                    */
/* ------------------------------------------------------------------ */

block_id_t mc_crafting_match(const block_id_t grid[9], uint8_t *out_count) {
    if (!grid || !out_count) {
        return BLOCK_AIR;
    }
    /* Nothing to match against an empty grid. */
    if (count_items(grid) == 0) {
        *out_count = 0;
        return BLOCK_AIR;
    }

    for (uint32_t i = 0; i < g_recipe_count; i++) {
        const mc_recipe_t *r = &g_recipes[i];
        int matched = 0;
        if (r->shapeless) {
            matched = match_shapeless(r, grid);
        } else {
            matched = match_shaped_all_orientations(r, grid);
        }
        if (matched) {
            *out_count = r->result_count;
            return r->result;
        }
    }

    *out_count = 0;
    return BLOCK_AIR;
}
