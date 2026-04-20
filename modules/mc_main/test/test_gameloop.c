#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mc_error.h"
#include "mc_types.h"

/* ---- Pull in internal header for state machine ---- */
#include "mc_main_internal.h"

/* ---- Error string tests ---- */

static void test_error_string_known_codes(void)
{
    assert(strcmp(mc_error_string(MC_OK),                "OK") == 0);
    assert(strcmp(mc_error_string(MC_ERR_INVALID_ARG),   "invalid argument") == 0);
    assert(strcmp(mc_error_string(MC_ERR_OUT_OF_MEMORY), "out of memory") == 0);
    assert(strcmp(mc_error_string(MC_ERR_NOT_FOUND),     "not found") == 0);
    assert(strcmp(mc_error_string(MC_ERR_ALREADY_EXISTS),"already exists") == 0);
    assert(strcmp(mc_error_string(MC_ERR_IO),            "I/O error") == 0);
    assert(strcmp(mc_error_string(MC_ERR_VULKAN),        "Vulkan error") == 0);
    assert(strcmp(mc_error_string(MC_ERR_PLATFORM),      "platform error") == 0);
    assert(strcmp(mc_error_string(MC_ERR_FULL),          "full") == 0);
    assert(strcmp(mc_error_string(MC_ERR_EMPTY),         "empty") == 0);
    assert(strcmp(mc_error_string(MC_ERR_NOT_READY),     "not ready") == 0);
    assert(strcmp(mc_error_string(MC_ERR_NETWORK),       "network error") == 0);
    assert(strcmp(mc_error_string(MC_ERR_PROTOCOL),      "protocol error") == 0);
    printf("    PASS test_error_string_known_codes\n");
}

static void test_error_string_unknown_code(void)
{
    assert(strcmp(mc_error_string(9999), "unknown error") == 0);
    assert(strcmp(mc_error_string(-1),   "unknown error") == 0);
    printf("    PASS test_error_string_unknown_code\n");
}

/* ---- State machine tests ---- */

static void test_state_init(void)
{
    mc_state_init();
    assert(mc_state_get() == GAME_STATE_MENU);
    printf("    PASS test_state_init\n");
}

static void test_state_set_get(void)
{
    mc_state_init();
    mc_state_set(GAME_STATE_PLAYING);
    assert(mc_state_get() == GAME_STATE_PLAYING);
    mc_state_set(GAME_STATE_PAUSED);
    assert(mc_state_get() == GAME_STATE_PAUSED);
    mc_state_set(GAME_STATE_LOADING);
    assert(mc_state_get() == GAME_STATE_LOADING);
    mc_state_set(GAME_STATE_MENU);
    assert(mc_state_get() == GAME_STATE_MENU);
    printf("    PASS test_state_set_get\n");
}

static void test_state_playing_to_paused(void)
{
    game_state_t next = mc_state_transition(GAME_STATE_PLAYING, 1);
    assert(next == GAME_STATE_PAUSED);
    printf("    PASS test_state_playing_to_paused\n");
}

static void test_state_paused_to_playing(void)
{
    game_state_t next = mc_state_transition(GAME_STATE_PAUSED, 1);
    assert(next == GAME_STATE_PLAYING);
    printf("    PASS test_state_paused_to_playing\n");
}

static void test_state_playing_no_pause(void)
{
    game_state_t next = mc_state_transition(GAME_STATE_PLAYING, 0);
    assert(next == GAME_STATE_PLAYING);
    printf("    PASS test_state_playing_no_pause\n");
}

static void test_state_paused_no_pause(void)
{
    game_state_t next = mc_state_transition(GAME_STATE_PAUSED, 0);
    assert(next == GAME_STATE_PAUSED);
    printf("    PASS test_state_paused_no_pause\n");
}

static void test_state_menu_ignores_pause(void)
{
    game_state_t next = mc_state_transition(GAME_STATE_MENU, 1);
    assert(next == GAME_STATE_MENU);
    printf("    PASS test_state_menu_ignores_pause\n");
}

static void test_state_loading_ignores_pause(void)
{
    game_state_t next = mc_state_transition(GAME_STATE_LOADING, 1);
    assert(next == GAME_STATE_LOADING);
    printf("    PASS test_state_loading_ignores_pause\n");
}

/* ---- Main ---- */

int main(void)
{
    printf("  mc_main tests:\n");

    test_error_string_known_codes();
    test_error_string_unknown_code();

    test_state_init();
    test_state_set_get();
    test_state_playing_to_paused();
    test_state_paused_to_playing();
    test_state_playing_no_pause();
    test_state_paused_no_pause();
    test_state_menu_ignores_pause();
    test_state_loading_ignores_pause();

    printf("  ALL mc_main tests passed\n");
    return 0;
}
