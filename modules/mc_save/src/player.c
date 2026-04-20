/*
 * player.c -- Player data save/load using NBT.
 *
 * Saved to <world_dir>/players/<name>.dat.
 */

#include "nbt.h"
#include "save_internal.h"
#include "mc_save.h"
#include "mc_types.h"
#include "mc_error.h"

#include <string.h>

mc_error_t mc_save_player(const char* name, vec3_t pos, float yaw, float pitch)
{
    if (!name || strlen(name) == 0) return MC_ERR_INVALID_ARG;
    if (g_world_dir[0] == '\0') return MC_ERR_IO;

    char path[SAVE_MAX_PATH];
    mc_error_t err = save_build_path(path, sizeof(path),
                                     "%s/players/%s.dat", g_world_dir, name);
    if (err) return err;

    nbt_tag_t* root = nbt_create_compound("Player");
    if (!root) return MC_ERR_OUT_OF_MEMORY;

    nbt_compound_add(root, nbt_create_string("Name", name));
    nbt_compound_add(root, nbt_create_float("PosX", pos.x));
    nbt_compound_add(root, nbt_create_float("PosY", pos.y));
    nbt_compound_add(root, nbt_create_float("PosZ", pos.z));
    nbt_compound_add(root, nbt_create_float("Yaw", yaw));
    nbt_compound_add(root, nbt_create_float("Pitch", pitch));

    err = nbt_write_file(path, root);
    nbt_free(root);
    return err;
}

mc_error_t mc_save_load_player(const char* name, vec3_t* pos, float* yaw, float* pitch)
{
    if (!name || !pos || !yaw || !pitch) return MC_ERR_INVALID_ARG;
    if (g_world_dir[0] == '\0') return MC_ERR_IO;

    char path[SAVE_MAX_PATH];
    mc_error_t err = save_build_path(path, sizeof(path),
                                     "%s/players/%s.dat", g_world_dir, name);
    if (err) return err;

    nbt_tag_t* root = NULL;
    err = nbt_read_file(path, &root);
    if (err) return err;

    if (!root || root->type != NBT_TAG_COMPOUND) {
        nbt_free(root);
        return MC_ERR_IO;
    }

    nbt_tag_t* px = nbt_compound_get(root, "PosX");
    nbt_tag_t* py = nbt_compound_get(root, "PosY");
    nbt_tag_t* pz = nbt_compound_get(root, "PosZ");
    nbt_tag_t* yw = nbt_compound_get(root, "Yaw");
    nbt_tag_t* pt = nbt_compound_get(root, "Pitch");

    if (!px || !py || !pz || !yw || !pt) {
        nbt_free(root);
        return MC_ERR_IO;
    }

    pos->x = px->payload.v_float;
    pos->y = py->payload.v_float;
    pos->z = pz->payload.v_float;
    pos->_pad = 0.0f;
    *yaw   = yw->payload.v_float;
    *pitch = pt->payload.v_float;

    nbt_free(root);
    return MC_OK;
}
