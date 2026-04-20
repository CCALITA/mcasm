/*
 * world_meta.c -- World metadata (level.dat) save/load using NBT.
 *
 * Saved to <world_dir>/level.dat.
 */

#include "nbt.h"
#include "save_internal.h"
#include "mc_save.h"
#include "mc_types.h"
#include "mc_error.h"

#include <string.h>

mc_error_t mc_save_world_meta(uint32_t seed, tick_t world_time, uint8_t game_mode)
{
    if (g_world_dir[0] == '\0') return MC_ERR_IO;

    char path[SAVE_MAX_PATH];
    mc_error_t err = save_build_path(path, sizeof(path),
                                     "%s/level.dat", g_world_dir);
    if (err) return err;

    nbt_tag_t* root = nbt_create_compound("Level");
    if (!root) return MC_ERR_OUT_OF_MEMORY;

    nbt_compound_add(root, nbt_create_int("Seed", (int32_t)seed));
    nbt_compound_add(root, nbt_create_int("Time", (int32_t)world_time));
    nbt_compound_add(root, nbt_create_byte("GameMode", (int8_t)game_mode));

    err = nbt_write_file(path, root);
    nbt_free(root);
    return err;
}

mc_error_t mc_save_load_world_meta(uint32_t* seed, tick_t* world_time, uint8_t* game_mode)
{
    if (!seed || !world_time || !game_mode) return MC_ERR_INVALID_ARG;
    if (g_world_dir[0] == '\0') return MC_ERR_IO;

    char path[SAVE_MAX_PATH];
    mc_error_t err = save_build_path(path, sizeof(path),
                                     "%s/level.dat", g_world_dir);
    if (err) return err;

    nbt_tag_t* root = NULL;
    err = nbt_read_file(path, &root);
    if (err) return err;

    if (!root || root->type != NBT_TAG_COMPOUND) {
        nbt_free(root);
        return MC_ERR_IO;
    }

    nbt_tag_t* s  = nbt_compound_get(root, "Seed");
    nbt_tag_t* t  = nbt_compound_get(root, "Time");
    nbt_tag_t* gm = nbt_compound_get(root, "GameMode");

    if (!s || !t || !gm) {
        nbt_free(root);
        return MC_ERR_IO;
    }

    *seed       = (uint32_t)s->payload.v_int;
    *world_time = (tick_t)t->payload.v_int;
    *game_mode  = (uint8_t)gm->payload.v_byte;

    nbt_free(root);
    return MC_OK;
}
