#include "command_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* /help - list all registered commands */
static mc_error_t cmd_help(const char** args, uint32_t argc,
                           char* out, uint32_t max)
{
    (void)args;
    (void)argc;

    const mc_command_entry_t* registry = mc_command_get_registry();
    uint32_t count = mc_command_count();

    int written = snprintf(out, max, "Available commands (%u):\n", count);
    if (written < 0) {
        return MC_ERR_IO;
    }

    uint32_t offset = (uint32_t)written;
    for (uint32_t i = 0; i < count && offset < max - 1; i++) {
        int n = snprintf(out + offset, max - offset, "  /%s - %s\n",
                         registry[i].name, registry[i].usage);
        if (n < 0) {
            break;
        }
        offset += (uint32_t)n;
    }

    return MC_OK;
}

/* /gamemode <mode> - switch game mode (0=survival, 1=creative) */
static mc_error_t cmd_gamemode(const char** args, uint32_t argc,
                               char* out, uint32_t max)
{
    if (argc < 1) {
        snprintf(out, max, "Usage: /gamemode <0|1>");
        return MC_ERR_INVALID_ARG;
    }

    int mode = atoi(args[0]);
    if (mode < 0 || mode > 1) {
        snprintf(out, max, "Invalid game mode. Use 0 (survival) or 1 (creative).");
        return MC_ERR_INVALID_ARG;
    }

    const char* name = (mode == 0) ? "survival" : "creative";
    snprintf(out, max, "Game mode set to %s", name);
    return MC_OK;
}

/* /tp <x> <y> <z> - teleport */
static mc_error_t cmd_tp(const char** args, uint32_t argc,
                         char* out, uint32_t max)
{
    if (argc < 3) {
        snprintf(out, max, "Usage: /tp <x> <y> <z>");
        return MC_ERR_INVALID_ARG;
    }

    float x = (float)atof(args[0]);
    float y = (float)atof(args[1]);
    float z = (float)atof(args[2]);

    snprintf(out, max, "Teleported to %.1f, %.1f, %.1f", x, y, z);
    return MC_OK;
}

/* /give <block_id> [count] - give items */
static mc_error_t cmd_give(const char** args, uint32_t argc,
                           char* out, uint32_t max)
{
    if (argc < 1) {
        snprintf(out, max, "Usage: /give <block_id> [count]");
        return MC_ERR_INVALID_ARG;
    }

    int block_id = atoi(args[0]);
    int count = 1;
    if (argc >= 2) {
        count = atoi(args[1]);
        if (count < 1) {
            count = 1;
        }
    }

    snprintf(out, max, "Gave %d of block %d", count, block_id);
    return MC_OK;
}

/* /time <set|add> <value> - set or add world time */
static mc_error_t cmd_time(const char** args, uint32_t argc,
                           char* out, uint32_t max)
{
    if (argc < 2) {
        snprintf(out, max, "Usage: /time <set|add> <value>");
        return MC_ERR_INVALID_ARG;
    }

    int value = atoi(args[1]);

    if (strcmp(args[0], "set") == 0) {
        snprintf(out, max, "Time set to %d", value);
    } else if (strcmp(args[0], "add") == 0) {
        snprintf(out, max, "Added %d to time", value);
    } else {
        snprintf(out, max, "Usage: /time <set|add> <value>");
        return MC_ERR_INVALID_ARG;
    }

    return MC_OK;
}

/* /seed - display world seed */
static mc_error_t cmd_seed(const char** args, uint32_t argc,
                           char* out, uint32_t max)
{
    (void)args;
    (void)argc;
    snprintf(out, max, "World seed: 12345");
    return MC_OK;
}

/* /kill - kill current player */
static mc_error_t cmd_kill(const char** args, uint32_t argc,
                           char* out, uint32_t max)
{
    (void)args;
    (void)argc;
    snprintf(out, max, "Player killed");
    return MC_OK;
}

mc_error_t mc_command_register_defaults(void)
{
    struct { const char* name; const char* usage; mc_command_fn handler; } cmds[] = {
        { "help",     "List all commands",                cmd_help     },
        { "gamemode", "/gamemode <0|1> - Set game mode",  cmd_gamemode },
        { "tp",       "/tp <x> <y> <z> - Teleport",      cmd_tp       },
        { "give",     "/give <block_id> [count] - Give items", cmd_give },
        { "time",     "/time <set|add> <value> - World time",  cmd_time },
        { "seed",     "/seed - Display world seed",       cmd_seed     },
        { "kill",     "/kill - Kill current player",      cmd_kill     },
    };

    for (uint32_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        mc_error_t err = mc_command_register(cmds[i].name, cmds[i].usage, cmds[i].handler);
        if (err != MC_OK) return err;
    }

    return MC_OK;
}
