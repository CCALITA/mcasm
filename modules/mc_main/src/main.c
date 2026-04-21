#include "mc_main_internal.h"

/* Module headers — ordered by init dependency */
#include "mc_memory.h"
#include "mc_math.h"
#include "mc_platform.h"
#include "mc_block.h"
#include "mc_world.h"
#include "mc_worldgen.h"
#include "mc_entity.h"
#include "mc_physics.h"
#include "mc_audio.h"
#include "mc_input.h"
#include "mc_ui.h"
#include "mc_render.h"
#include "mc_net.h"
#include "mc_save.h"
#include "mc_crafting.h"
#include "mc_redstone.h"
#include "mc_particle.h"
#include "mc_command.h"
#include "mc_mob_ai.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Defaults ---- */

#define DEFAULT_SEED            12345
#define DEFAULT_WIDTH           800
#define DEFAULT_HEIGHT          600
#define DEFAULT_RENDER_DISTANCE 4

/* ---- Command-line parsing ---- */

static game_config_t parse_args(int argc, char *argv[])
{
    game_config_t cfg = {
        .seed            = DEFAULT_SEED,
        .window_width    = DEFAULT_WIDTH,
        .window_height   = DEFAULT_HEIGHT,
        .render_distance = DEFAULT_RENDER_DISTANCE
    };

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            cfg.seed = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            cfg.window_width = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            cfg.window_height = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--render-distance") == 0 && i + 1 < argc) {
            cfg.render_distance = (int32_t)strtol(argv[++i], NULL, 10);
        }
    }

    return cfg;
}

/* ---- Helpers ---- */

#define CHECK_INIT(call, label)           \
    do {                                  \
        mc_error_t _err = (call);         \
        if (_err != MC_OK) {              \
            fprintf(stderr, "FATAL: %s failed: %s\n", \
                    (label), mc_error_string(_err)); \
            goto shutdown;                \
        }                                 \
    } while (0)

/* ---- Entry point ---- */

int main(int argc, char *argv[])
{
    game_config_t config = parse_args(argc, argv);
    mc_error_t err = MC_OK;

    /* Track how far init progressed for reverse-order shutdown */
    int init_stage = 0;

    /* ---- Initialize modules in dependency order ---- */

    CHECK_INIT(mc_memory_init(),  "mc_memory_init");
    init_stage = 1;

    /* Init save early so we can load world meta before mc_world_init */
    CHECK_INIT(mc_save_init("world"), "mc_save_init");
    init_stage = 2;

    /* If a saved world exists, override the command-line seed */
    {
        uint32_t saved_seed = 0;
        tick_t   saved_time = 0;
        uint8_t  saved_mode = 0;
        if (mc_save_load_world_meta(&saved_seed, &saved_time, &saved_mode) == MC_OK) {
            config.seed = saved_seed;
        }
    }

    /* mc_math has no init — pure functions */

    CHECK_INIT(mc_platform_init(config.window_width, config.window_height, "mcasm"),
               "mc_platform_init");
    init_stage = 3;

    CHECK_INIT(mc_block_init(),   "mc_block_init");
    init_stage = 4;

    CHECK_INIT(mc_world_init(config.seed), "mc_world_init");
    init_stage = 5;

    CHECK_INIT(mc_worldgen_init(config.seed), "mc_worldgen_init");
    init_stage = 6;

    CHECK_INIT(mc_entity_init(),  "mc_entity_init");
    init_stage = 7;

    /* Wire physics to world block query */
    CHECK_INIT(mc_physics_init(mc_world_get_block), "mc_physics_init");
    init_stage = 8;

    CHECK_INIT(mc_audio_init(),   "mc_audio_init");
    init_stage = 9;

    CHECK_INIT(mc_input_init(),   "mc_input_init");
    init_stage = 10;

    CHECK_INIT(mc_ui_init(config.window_width, config.window_height),
               "mc_ui_init");
    init_stage = 11;

    {
        void *win = mc_platform_get_window_handle();
        uint32_t fb_w = 0, fb_h = 0;
        mc_platform_get_framebuffer_size(&fb_w, &fb_h);
        CHECK_INIT(mc_render_init(win, fb_w, fb_h), "mc_render_init");
    }
    init_stage = 12;

    CHECK_INIT(mc_net_init(),     "mc_net_init");
    init_stage = 13;

    CHECK_INIT(mc_crafting_init(), "mc_crafting_init");
    init_stage = 14;

    /* Wire redstone to world get/set block */
    CHECK_INIT(mc_redstone_init(mc_world_get_block, mc_world_set_block),
               "mc_redstone_init");
    init_stage = 15;

    CHECK_INIT(mc_particle_init(), "mc_particle_init");
    init_stage = 16;

    CHECK_INIT(mc_command_init(),  "mc_command_init");
    init_stage = 17;

    /* Wire mob AI to world block query */
    CHECK_INIT(mc_mob_ai_init(mc_world_get_block), "mc_mob_ai_init");
    init_stage = 18;

    /* ---- Initialize state machine and run game loop ---- */

    mc_state_init();
    err = mc_game_loop_run(&config);

shutdown:
    /* ---- Shutdown in reverse order ---- */
    if (init_stage >= 18) mc_mob_ai_shutdown();
    if (init_stage >= 17) mc_command_shutdown();
    if (init_stage >= 16) mc_particle_shutdown();
    if (init_stage >= 15) mc_redstone_shutdown();
    if (init_stage >= 14) mc_crafting_shutdown();
    if (init_stage >= 13) mc_net_shutdown();
    if (init_stage >= 12) mc_render_shutdown();
    if (init_stage >= 11) mc_ui_shutdown();
    if (init_stage >= 10) mc_input_shutdown();
    if (init_stage >=  9) mc_audio_shutdown();
    if (init_stage >=  8) mc_physics_shutdown();
    if (init_stage >=  7) mc_entity_shutdown();
    if (init_stage >=  6) mc_worldgen_shutdown();
    if (init_stage >=  5) mc_world_shutdown();
    if (init_stage >=  4) mc_block_shutdown();
    if (init_stage >=  3) mc_platform_shutdown();
    if (init_stage >=  2) mc_save_shutdown();
    if (init_stage >=  1) mc_memory_shutdown();

    return (err == MC_OK) ? 0 : 1;
}
