#include "mc_main_internal.h"

#include "mc_platform.h"
#include "mc_input.h"
#include "mc_physics.h"
#include "mc_entity.h"
#include "mc_world.h"
#include "mc_redstone.h"
#include "mc_particle.h"
#include "mc_render.h"
#include "mc_audio.h"
#include "mc_ui.h"
#include "mc_math.h"
#include "mc_mob_ai.h"

#include <math.h>

/* ---- Internal tick counter ---- */

static tick_t g_current_tick = 0;

/* ---- Fixed-timestep helpers ---- */

static void tick_update(void)
{
    const float dt = (float)TICK_INTERVAL;

    mc_input_update();

    /* State machine transition on pause key */
    game_state_t state = mc_state_get();
    uint8_t pause_pressed = mc_input_action_pressed(ACTION_PAUSE);
    game_state_t next = mc_state_transition(state, pause_pressed);
    if (next != state) {
        mc_state_set(next);
    }

    /* Only simulate when playing */
    if (mc_state_get() != GAME_STATE_PLAYING) {
        return;
    }

    mc_physics_tick(dt);

    /* Block interaction (dig / place) for player entity 1 */
    {
        transform_component_t *ptf = mc_entity_get_transform(1);
        if (ptf) {
            interaction_update(1, ptf->yaw, ptf->pitch);
        }
    }

    mc_world_tick(g_current_tick);
    mc_mob_ai_tick(g_current_tick);
    mc_redstone_tick(g_current_tick);
    mc_particle_tick(dt);
    mc_audio_tick();

    g_current_tick++;
}

static void render_frame(void)
{
    uint32_t fb_w = 0, fb_h = 0;
    mc_platform_get_framebuffer_size(&fb_w, &fb_h);

    /* Build view and projection matrices from entity 1 (player) */
    mat4_t view = mc_math_mat4_identity();
    float aspect = (fb_h > 0) ? (float)fb_w / (float)fb_h : 1.0f;
    mat4_t projection = mc_math_mat4_perspective(1.22173f, aspect, 0.1f, 1000.0f);

    transform_component_t *player_tf = mc_entity_get_transform(1);
    if (player_tf) {
        vec3_t eye = player_tf->position;
        float yaw   = player_tf->yaw;
        float pitch = player_tf->pitch;

        /* Compute forward vector from yaw/pitch */
        float cos_p = cosf(pitch);
        vec3_t center = {
            eye.x + sinf(yaw) * cos_p,
            eye.y + sinf(pitch),
            eye.z - cosf(yaw) * cos_p,
            0.0f
        };
        vec3_t up = {0.0f, 1.0f, 0.0f, 0.0f};
        view = mc_math_mat4_look_at(eye, center, up);
    }

    mc_render_begin_frame(&view, &projection);
    mc_render_draw_terrain(NULL, 0);
    mc_render_draw_entities(NULL, NULL, 0);
    mc_render_draw_ui();
    mc_render_end_frame();
}

/* ---- Public game loop entry point ---- */

mc_error_t mc_game_loop_run(const game_config_t *config)
{
    /* Transition straight into PLAYING for now (skip menu/loading) */
    mc_state_set(GAME_STATE_LOADING);

    /* Generate initial chunks around origin */
    int32_t rd = config->render_distance;
    for (int32_t cx = -rd; cx <= rd; cx++) {
        for (int32_t cz = -rd; cz <= rd; cz++) {
            chunk_pos_t pos = {cx, cz};
            mc_world_load_chunk(pos);
        }
    }

    /* Create player entity with transform + physics + player components */
    entity_id_t player = mc_entity_create(
        COMPONENT_TRANSFORM | COMPONENT_PHYSICS | COMPONENT_PLAYER
    );
    if (player != ENTITY_INVALID) {
        int32_t spawn_y = mc_world_get_height(0, 0) + 2;
        vec3_t spawn = {0.0f, (float)spawn_y, 0.0f, 0.0f};
        mc_entity_set_position(player, spawn);
    }

    mc_state_set(GAME_STATE_PLAYING);

    /* Fixed-timestep accumulator loop */
    double previous = mc_platform_get_time();
    double accumulator = 0.0;

    while (!mc_platform_should_close()) {
        double current = mc_platform_get_time();
        double frame_time = current - previous;
        previous = current;

        /* Clamp to avoid spiral of death */
        if (frame_time > MAX_FRAME_TIME) {
            frame_time = MAX_FRAME_TIME;
        }

        accumulator += frame_time;

        /* Consume time in tick-sized chunks */
        while (accumulator >= TICK_INTERVAL) {
            tick_update();
            accumulator -= TICK_INTERVAL;
        }

        mc_platform_poll_events();
        render_frame();
    }

    /* Clean up player */
    if (player != ENTITY_INVALID) {
        mc_entity_destroy(player);
    }

    return MC_OK;
}
