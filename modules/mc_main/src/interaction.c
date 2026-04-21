#include "mc_main_internal.h"

#include "mc_input.h"
#include "mc_physics.h"
#include "mc_entity.h"
#include "mc_world.h"
#include "mc_block.h"

#include <math.h>

/* Eye offset above entity position (Minecraft standard) */
#define EYE_OFFSET_Y 1.62f

/* Maximum reach distance for block interaction */
#define REACH_DISTANCE 5.0f

/* Hardcoded selected block (hotbar placeholder) */
static block_id_t g_selected_block = BLOCK_STONE;

void interaction_init(void)
{
    g_selected_block = BLOCK_STONE;
}

void interaction_update(entity_id_t player, float yaw, float pitch)
{
    uint8_t dig   = mc_input_action_pressed(ACTION_DIG);
    uint8_t place = mc_input_action_pressed(ACTION_PLACE);

    if (!dig && !place) {
        return;
    }

    transform_component_t *tf = mc_entity_get_transform(player);
    if (!tf) {
        return;
    }

    vec3_t eye = {
        tf->position.x,
        tf->position.y + EYE_OFFSET_Y,
        tf->position.z,
        0.0f
    };

    float cos_p = cosf(pitch);
    vec3_t direction = {
        -sinf(yaw) * cos_p,
         sinf(pitch),
        -cosf(yaw) * cos_p,
        0.0f
    };

    raycast_result_t result = mc_physics_raycast(eye, direction, REACH_DISTANCE);
    if (!result.hit) {
        return;
    }

    if (dig) {
        mc_world_set_block(result.block_pos, BLOCK_AIR);
    }

    if (place) {
        block_pos_t adjacent = {
            result.block_pos.x + (int32_t)result.hit_normal.x,
            result.block_pos.y + (int32_t)result.hit_normal.y,
            result.block_pos.z + (int32_t)result.hit_normal.z,
            0
        };
        mc_world_set_block(adjacent, g_selected_block);
    }
}
