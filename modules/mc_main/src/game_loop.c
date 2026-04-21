#include "mc_main_internal.h"

#include "mc_platform.h"
#include "mc_input.h"
#include "mc_physics.h"
#include "mc_entity.h"
#include "mc_world.h"
#include "mc_worldgen.h"
#include "mc_redstone.h"
#include "mc_particle.h"
#include "mc_render.h"
#include "mc_audio.h"
#include "mc_ui.h"
#include "mc_math.h"
#include "mc_mob_ai.h"

#include <math.h>
#include <stdio.h>

/* ---- Constants ---- */

#define PLAYER_MOVE_SPEED  4.317f   /* Minecraft walk speed (m/s) */
#define PLAYER_JUMP_SPEED  8.0f
#define GRAVITY            20.0f
#define HALF_PI            1.5707963f
#define PLAYER_EYE_HEIGHT  1.62f
#define PLAYER_HEIGHT      1.80f

/* Player AABB relative to feet position */
static const aabb_t PLAYER_AABB_TEMPLATE = {
    .min = { -0.3f, 0.0f, -0.3f, 0.0f },
    .max = {  0.3f, PLAYER_HEIGHT, 0.3f, 0.0f }
};

/* ---- Module state ---- */

static tick_t      g_current_tick  = 0;
static entity_id_t g_player_entity = ENTITY_INVALID;

/* ---- Helpers ---- */

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static aabb_t player_aabb_at(vec3_t pos)
{
    aabb_t box;
    box.min = mc_math_vec3_add(pos, PLAYER_AABB_TEMPLATE.min);
    box.max = mc_math_vec3_add(pos, PLAYER_AABB_TEMPLATE.max);
    return box;
}

/* ---- Player movement (called once per tick while playing) ---- */

static void update_player(float dt)
{
    if (g_player_entity == ENTITY_INVALID) {
        return;
    }

    transform_component_t *tf = mc_entity_get_transform(g_player_entity);
    physics_component_t   *ph = mc_entity_get_physics(g_player_entity);
    if (!tf || !ph) {
        return;
    }

    float dyaw  = 0.0f;
    float dpitch = 0.0f;
    mc_input_get_look_delta(&dyaw, &dpitch);
    tf->yaw  += dyaw;
    tf->pitch = clampf(tf->pitch + dpitch, -HALF_PI, HALF_PI);

    float fwd    = mc_input_get_move_forward();
    float strafe = mc_input_get_move_strafe();

    float yaw = tf->yaw;
    vec3_t forward = { -sinf(yaw), 0.0f, -cosf(yaw), 0.0f };
    vec3_t up      = {  0.0f,      1.0f,  0.0f,       0.0f };
    vec3_t right   = mc_math_vec3_cross(forward, up);

    vec3_t move_dir = mc_math_vec3_add(
        mc_math_vec3_scale(forward, fwd),
        mc_math_vec3_scale(right, strafe)
    );
    vec3_t vel = mc_math_vec3_scale(move_dir, PLAYER_MOVE_SPEED);

    /* Carry vertical velocity forward; jump resets it, gravity accumulates */
    vel.y = tf->velocity.y;

    if (mc_input_action_pressed(ACTION_JUMP) && ph->on_ground) {
        vel.y = PLAYER_JUMP_SPEED;
    }

    if (!ph->on_ground) {
        vel.y -= GRAVITY * dt;
    }

    aabb_t box      = player_aabb_at(tf->position);
    vec3_t resolved = mc_physics_move_and_slide(box, vel, dt);

    tf->position = mc_math_vec3_add(tf->position, resolved);
    tf->velocity = vel;
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

    update_player(dt);

    mc_physics_tick(dt);
    mc_world_tick(g_current_tick);
    mc_mob_ai_tick(g_current_tick);
    mc_redstone_tick(g_current_tick);
    mc_particle_tick(dt);
    mc_audio_tick();

    g_current_tick++;
}

/* ---- Mesh tracking ---- */

#define MAX_TRACKED_MESHES 8192
static uint64_t g_mesh_handles[MAX_TRACKED_MESHES];
static uint32_t g_mesh_count = 0;

static void build_all_chunk_meshes(void)
{
    g_mesh_count = 0;

    chunk_pos_t dirty[256];
    uint32_t dirty_count = 0;
    mc_world_get_dirty_chunks(dirty, 256, &dirty_count);

    for (uint32_t i = 0; i < dirty_count && g_mesh_count < MAX_TRACKED_MESHES; i++) {
        const chunk_t *chunk = mc_world_get_chunk(dirty[i]);
        if (!chunk) continue;

        for (uint8_t sy = 0; sy < SECTION_COUNT; sy++) {
            if (chunk->sections[sy].non_air_count == 0) continue;
            if (g_mesh_count >= MAX_TRACKED_MESHES) break;

            uint64_t h = mc_render_build_chunk_mesh(&chunk->sections[sy], dirty[i], sy);
            if (h != 0) {
                g_mesh_handles[g_mesh_count++] = h;
            }
        }
    }
}

static void render_frame(void)
{
    uint32_t fb_w = 0, fb_h = 0;
    mc_platform_get_framebuffer_size(&fb_w, &fb_h);

    /* Build view and projection matrices from the player entity */
    mat4_t view = mc_math_mat4_identity();
    float aspect = (fb_h > 0) ? (float)fb_w / (float)fb_h : 1.0f;
    mat4_t projection = mc_math_mat4_perspective(1.22173f, aspect, 0.1f, 1000.0f);

    transform_component_t *player_tf = mc_entity_get_transform(g_player_entity);
    if (player_tf) {
        vec3_t eye = player_tf->position;
        eye.y += PLAYER_EYE_HEIGHT;

        float yaw   = player_tf->yaw;
        float pitch = player_tf->pitch;

        /* Look direction from yaw/pitch */
        float cos_p = cosf(pitch);
        vec3_t center = {
            eye.x - sinf(yaw) * cos_p,
            eye.y + sinf(pitch),
            eye.z - cosf(yaw) * cos_p,
            0.0f
        };
        vec3_t up = {0.0f, 1.0f, 0.0f, 0.0f};
        view = mc_math_mat4_look_at(eye, center, up);
    }

    mc_render_begin_frame(&view, &projection);
    mc_render_draw_terrain(g_mesh_handles, g_mesh_count);
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
    fprintf(stderr, "Generating %d chunks...\n", (2*rd+1)*(2*rd+1));
    for (int32_t cx = -rd; cx <= rd; cx++) {
        for (int32_t cz = -rd; cz <= rd; cz++) {
            chunk_pos_t pos = {cx, cz};
            mc_world_load_chunk(pos);

            /* Generate terrain into the chunk */
            chunk_t *chunk = (chunk_t*)mc_world_get_chunk(pos);
            if (chunk) {
                mc_worldgen_generate_chunk(pos, chunk);
            }
        }
    }
    fprintf(stderr, "Chunks generated. Building meshes...\n");

    /* Build initial meshes from all chunks */
    build_all_chunk_meshes();
    fprintf(stderr, "Built %u meshes.\n", g_mesh_count);
    fflush(stderr);

    /* Create player entity with transform + physics + player components */
    g_player_entity = mc_entity_create(
        COMPONENT_TRANSFORM | COMPONENT_PHYSICS | COMPONENT_PLAYER
    );
    if (g_player_entity != ENTITY_INVALID) {
        int32_t spawn_y = mc_world_get_height(8, 8) + 2;
        vec3_t spawn = {8.0f, (float)spawn_y, 8.0f, 0.0f};
        mc_entity_set_position(g_player_entity, spawn);
        fprintf(stderr, "Player spawned at (8, %d, 8)\n", spawn_y);
    }

    mc_state_set(GAME_STATE_PLAYING);

    /* Lock cursor for FPS-style mouse look */
    mc_platform_set_cursor_locked(1);

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
    if (g_player_entity != ENTITY_INVALID) {
        mc_entity_destroy(g_player_entity);
        g_player_entity = ENTITY_INVALID;
    }

    return MC_OK;
}
