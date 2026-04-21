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
#include "mc_save.h"

#include <math.h>

/* ---- Internal tick counter ---- */

static tick_t g_current_tick = 0;

/* ---- Auto-save interval (~5 minutes at 20 ticks/sec) ---- */

#define AUTOSAVE_INTERVAL 6000
#define MAX_DIRTY_SAVE    256

static void save_dirty_chunks(void)
{
    chunk_pos_t dirty[MAX_DIRTY_SAVE];
    uint32_t    dirty_count = 0;

    mc_world_get_dirty_chunks(dirty, MAX_DIRTY_SAVE, &dirty_count);
    for (uint32_t i = 0; i < dirty_count; i++) {
        const chunk_t *chunk = mc_world_get_chunk(dirty[i]);
        if (chunk) {
            mc_save_chunk(dirty[i], chunk);
        }
    }
}

static void save_player_state(entity_id_t player)
{
    transform_component_t *tf = mc_entity_get_transform(player);
    if (tf) {
        mc_save_player("player", tf->position, tf->yaw, tf->pitch);
    }
}

static void autosave(entity_id_t player, uint32_t seed)
{
    save_dirty_chunks();
    save_player_state(player);
    mc_save_world_meta(seed, g_current_tick, 0);
}

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

    uint64_t *mesh_handles = NULL;
    uint32_t  mesh_count   = 0;
    chunk_mesh_mgr_get_meshes(&mesh_handles, &mesh_count);
    mc_render_draw_terrain(mesh_handles, mesh_count);

    mc_render_draw_entities(NULL, NULL, 0);
    mc_render_draw_ui();
    mc_render_end_frame();
}

/* ---- Public game loop entry point ---- */

mc_error_t mc_game_loop_run(const game_config_t *config)
{
    /* Transition straight into LOADING (skip menu for now) */
    mc_state_set(GAME_STATE_LOADING);

    /* Load or generate initial chunks around origin */
    int32_t rd = config->render_distance;
    for (int32_t cx = -rd; cx <= rd; cx++) {
        for (int32_t cz = -rd; cz <= rd; cz++) {
            chunk_pos_t pos = {cx, cz};
            mc_world_load_chunk(pos);

            /* Try restoring saved block data over the generated chunk */
            chunk_t saved;
            if (mc_save_load_chunk(pos, &saved) == MC_OK) {
                for (uint8_t sy = 0; sy < SECTION_COUNT; sy++) {
                    mc_world_fill_section(pos, sy, saved.sections[sy].blocks);
                }
            }
        }
    }

    chunk_mesh_mgr_init();

    /* Create player entity with transform + physics + player components */
    entity_id_t player = mc_entity_create(
        COMPONENT_TRANSFORM | COMPONENT_PHYSICS | COMPONENT_PLAYER
    );
    if (player != ENTITY_INVALID) {
        /* Try restoring saved player position, otherwise use spawn */
        vec3_t saved_pos = {0};
        float  saved_yaw = 0.0f;
        float  saved_pitch = 0.0f;
        if (mc_save_load_player("player", &saved_pos, &saved_yaw, &saved_pitch) == MC_OK) {
            mc_entity_set_position(player, saved_pos);
            transform_component_t *tf = mc_entity_get_transform(player);
            if (tf) {
                tf->yaw   = saved_yaw;
                tf->pitch = saved_pitch;
            }
        } else {
            int32_t spawn_y = mc_world_get_height(0, 0) + 2;
            vec3_t spawn = {0.0f, (float)spawn_y, 0.0f, 0.0f};
            mc_entity_set_position(player, spawn);
        }
    }

    mc_state_set(GAME_STATE_PLAYING);

    /* Fixed-timestep accumulator loop */
    double previous = mc_platform_get_time();
    double accumulator = 0.0;
    tick_t last_autosave = g_current_tick;

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

        /* Auto-save periodically */
        if (g_current_tick - last_autosave >= AUTOSAVE_INTERVAL) {
            if (player != ENTITY_INVALID) {
                autosave(player, config->seed);
            }
            last_autosave = g_current_tick;
        }

        mc_platform_poll_events();
        chunk_mesh_mgr_update();
        render_frame();
    }

    /* ---- Shutdown save: persist everything before exit ---- */
    if (player != ENTITY_INVALID) {
        autosave(player, config->seed);
        mc_entity_destroy(player);
    }

    chunk_mesh_mgr_shutdown();

    return MC_OK;
}
