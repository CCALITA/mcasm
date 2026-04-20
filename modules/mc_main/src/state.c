#include "mc_main_internal.h"

static game_state_t g_state = GAME_STATE_MENU;

void mc_state_init(void)
{
    g_state = GAME_STATE_MENU;
}

game_state_t mc_state_get(void)
{
    return g_state;
}

void mc_state_set(game_state_t state)
{
    if (state < GAME_STATE_COUNT) {
        g_state = state;
    }
}

game_state_t mc_state_transition(game_state_t current, uint8_t pause_pressed)
{
    switch (current) {
    case GAME_STATE_MENU:
        /* Menu -> Loading is driven by main.c, not pause key */
        return GAME_STATE_MENU;

    case GAME_STATE_LOADING:
        /* Loading -> Playing happens when load completes (driven externally) */
        return GAME_STATE_LOADING;

    case GAME_STATE_PLAYING:
        if (pause_pressed) {
            return GAME_STATE_PAUSED;
        }
        return GAME_STATE_PLAYING;

    case GAME_STATE_PAUSED:
        if (pause_pressed) {
            return GAME_STATE_PLAYING;
        }
        return GAME_STATE_PAUSED;

    default:
        return current;
    }
}
