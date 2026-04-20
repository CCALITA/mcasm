#ifndef MC_MAIN_INTERNAL_H
#define MC_MAIN_INTERNAL_H

#include "mc_types.h"
#include "mc_error.h"

/* ---- Game states ---- */

typedef enum {
    GAME_STATE_MENU    = 0,
    GAME_STATE_LOADING = 1,
    GAME_STATE_PLAYING = 2,
    GAME_STATE_PAUSED  = 3,
    GAME_STATE_COUNT   = 4
} game_state_t;

/* ---- Game config (parsed from command line) ---- */

typedef struct {
    uint32_t seed;
    uint32_t window_width;
    uint32_t window_height;
    int32_t  render_distance;
} game_config_t;

/* ---- State machine (state.c) ---- */

void         mc_state_init(void);
game_state_t mc_state_get(void);
void         mc_state_set(game_state_t state);
game_state_t mc_state_transition(game_state_t current, uint8_t pause_pressed);

/* ---- Game loop (game_loop.c) ---- */

#define TICK_RATE      20
#define TICK_INTERVAL  (1.0 / (double)TICK_RATE)   /* 50 ms */
#define MAX_FRAME_TIME 0.25  /* clamp to avoid spiral of death */

mc_error_t mc_game_loop_run(const game_config_t *config);

/* ---- Error (error.c — public API already in mc_error.h) ---- */

#endif /* MC_MAIN_INTERNAL_H */
