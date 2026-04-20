#include "mc_command.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define RESPONSE_MAX 1024

static int s_custom_called = 0;
static uint32_t s_custom_argc = 0;
static char s_custom_args[4][64];

static mc_error_t custom_handler(const char** args, uint32_t argc,
                                 char* out, uint32_t max)
{
    s_custom_called = 1;
    s_custom_argc = argc;
    for (uint32_t i = 0; i < argc && i < 4; i++) {
        strncpy(s_custom_args[i], args[i], 63);
        s_custom_args[i][63] = '\0';
    }
    snprintf(out, max, "custom ok");
    return MC_OK;
}

static void test_register_and_execute(void)
{
    printf("  test_register_and_execute...");

    mc_command_shutdown();
    mc_command_init();

    s_custom_called = 0;
    s_custom_argc = 0;

    mc_error_t err = mc_command_register("mycmd", "A custom command", custom_handler);
    assert(err == MC_OK);

    char response[RESPONSE_MAX];
    err = mc_command_execute("/mycmd hello world", response, RESPONSE_MAX);
    assert(err == MC_OK);
    assert(s_custom_called == 1);
    assert(s_custom_argc == 2);
    assert(strcmp(s_custom_args[0], "hello") == 0);
    assert(strcmp(s_custom_args[1], "world") == 0);
    assert(strcmp(response, "custom ok") == 0);

    printf(" PASS\n");
}

static void test_parse_multiple_args(void)
{
    printf("  test_parse_multiple_args...");

    mc_command_shutdown();
    mc_command_init();

    s_custom_called = 0;
    s_custom_argc = 0;

    mc_command_register("multi", "Multi-arg test", custom_handler);

    char response[RESPONSE_MAX];
    mc_error_t err = mc_command_execute("/multi a b c d", response, RESPONSE_MAX);
    assert(err == MC_OK);
    assert(s_custom_argc == 4);
    assert(strcmp(s_custom_args[0], "a") == 0);
    assert(strcmp(s_custom_args[1], "b") == 0);
    assert(strcmp(s_custom_args[2], "c") == 0);
    assert(strcmp(s_custom_args[3], "d") == 0);

    printf(" PASS\n");
}

static void test_quoted_args(void)
{
    printf("  test_quoted_args...");

    mc_command_shutdown();
    mc_command_init();

    s_custom_called = 0;
    s_custom_argc = 0;

    mc_command_register("quote", "Quoted test", custom_handler);

    char response[RESPONSE_MAX];
    mc_error_t err = mc_command_execute("/quote \"hello world\" end", response, RESPONSE_MAX);
    assert(err == MC_OK);
    assert(s_custom_argc == 2);
    assert(strcmp(s_custom_args[0], "hello world") == 0);
    assert(strcmp(s_custom_args[1], "end") == 0);

    printf(" PASS\n");
}

static void test_tab_completion(void)
{
    printf("  test_tab_completion...");

    mc_command_shutdown();
    mc_command_init();

    /* Default commands include "help", "gamemode", "tp", "give", "time", "seed", "kill" */
    const char* completions[16];
    uint32_t count;

    /* Complete "t" should match "tp" and "time" */
    count = mc_command_complete("/t", completions, 16);
    assert(count == 2);

    /* Complete "he" should match "help" */
    count = mc_command_complete("/he", completions, 16);
    assert(count == 1);
    assert(strcmp(completions[0], "help") == 0);

    /* Complete with no prefix should return all */
    count = mc_command_complete("/", completions, 16);
    assert(count == 7); /* 7 default commands */

    printf(" PASS\n");
}

static void test_unknown_command(void)
{
    printf("  test_unknown_command...");

    mc_command_shutdown();
    mc_command_init();

    char response[RESPONSE_MAX];
    mc_error_t err = mc_command_execute("/nonexistent", response, RESPONSE_MAX);
    assert(err == MC_ERR_NOT_FOUND);
    assert(strstr(response, "Unknown command") != NULL);

    printf(" PASS\n");
}

static void test_invalid_input(void)
{
    printf("  test_invalid_input...");

    mc_command_shutdown();
    mc_command_init();

    char response[RESPONSE_MAX];

    /* No leading slash */
    mc_error_t err = mc_command_execute("help", response, RESPONSE_MAX);
    assert(err == MC_ERR_INVALID_ARG);

    /* Empty string */
    err = mc_command_execute("", response, RESPONSE_MAX);
    assert(err == MC_ERR_INVALID_ARG);

    printf(" PASS\n");
}

static void test_default_commands(void)
{
    printf("  test_default_commands...");

    mc_command_shutdown();
    mc_command_init();

    char response[RESPONSE_MAX];
    mc_error_t err;

    /* /help */
    err = mc_command_execute("/help", response, RESPONSE_MAX);
    assert(err == MC_OK);
    assert(strstr(response, "Available commands") != NULL);

    /* /gamemode 1 */
    err = mc_command_execute("/gamemode 1", response, RESPONSE_MAX);
    assert(err == MC_OK);
    assert(strstr(response, "creative") != NULL);

    /* /tp 1.5 2.5 3.5 */
    err = mc_command_execute("/tp 1.5 2.5 3.5", response, RESPONSE_MAX);
    assert(err == MC_OK);
    assert(strstr(response, "1.5") != NULL);
    assert(strstr(response, "2.5") != NULL);
    assert(strstr(response, "3.5") != NULL);

    /* /give 42 10 */
    err = mc_command_execute("/give 42 10", response, RESPONSE_MAX);
    assert(err == MC_OK);
    assert(strstr(response, "10") != NULL);
    assert(strstr(response, "42") != NULL);

    /* /time set 6000 */
    err = mc_command_execute("/time set 6000", response, RESPONSE_MAX);
    assert(err == MC_OK);
    assert(strstr(response, "6000") != NULL);

    /* /seed */
    err = mc_command_execute("/seed", response, RESPONSE_MAX);
    assert(err == MC_OK);
    assert(strstr(response, "seed") != NULL || strstr(response, "Seed") != NULL);

    /* /kill */
    err = mc_command_execute("/kill", response, RESPONSE_MAX);
    assert(err == MC_OK);
    assert(strstr(response, "killed") != NULL || strstr(response, "Killed") != NULL);

    printf(" PASS\n");
}

static void test_duplicate_register(void)
{
    printf("  test_duplicate_register...");

    mc_command_shutdown();
    mc_command_init();

    mc_error_t err = mc_command_register("help", "duplicate", custom_handler);
    assert(err == MC_ERR_ALREADY_EXISTS);

    printf(" PASS\n");
}

static void test_command_count(void)
{
    printf("  test_command_count...");

    mc_command_shutdown();
    mc_command_init();

    uint32_t count = mc_command_count();
    assert(count == 7); /* 7 default commands */

    mc_command_register("extra", "Extra command", custom_handler);
    assert(mc_command_count() == 8);

    printf(" PASS\n");
}

int main(void)
{
    printf("mc_command tests:\n");

    test_register_and_execute();
    test_parse_multiple_args();
    test_quoted_args();
    test_tab_completion();
    test_unknown_command();
    test_invalid_input();
    test_default_commands();
    test_duplicate_register();
    test_command_count();

    printf("All mc_command tests passed.\n");
    return 0;
}
