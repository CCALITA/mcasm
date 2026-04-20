#include "command_internal.h"
#include <string.h>
#include <stdio.h>

static mc_command_entry_t s_commands[MC_COMMAND_MAX];
static uint32_t           s_count = 0;

const mc_command_entry_t* mc_command_get_registry(void)
{
    return s_commands;
}

mc_error_t mc_command_init(void)
{
    s_count = 0;
    memset(s_commands, 0, sizeof(s_commands));
    return mc_command_register_defaults();
}

void mc_command_shutdown(void)
{
    s_count = 0;
    memset(s_commands, 0, sizeof(s_commands));
}

mc_error_t mc_command_register(const char* name, const char* usage, mc_command_fn handler)
{
    if (name == NULL || handler == NULL) {
        return MC_ERR_INVALID_ARG;
    }

    if (s_count >= MC_COMMAND_MAX) {
        return MC_ERR_FULL;
    }

    for (uint32_t i = 0; i < s_count; i++) {
        if (strcmp(s_commands[i].name, name) == 0) {
            return MC_ERR_ALREADY_EXISTS;
        }
    }

    mc_command_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    strncpy(entry.name, name, MC_COMMAND_NAME_MAX - 1);
    entry.name[MC_COMMAND_NAME_MAX - 1] = '\0';

    if (usage != NULL) {
        strncpy(entry.usage, usage, MC_COMMAND_USAGE_MAX - 1);
        entry.usage[MC_COMMAND_USAGE_MAX - 1] = '\0';
    }

    entry.handler = handler;
    s_commands[s_count++] = entry;

    return MC_OK;
}

mc_error_t mc_command_execute(const char* input, char* out_response, uint32_t max_response)
{
    if (input == NULL || out_response == NULL || max_response == 0) {
        return MC_ERR_INVALID_ARG;
    }

    out_response[0] = '\0';

    char       name[MC_COMMAND_NAME_MAX];
    const char* args[MC_COMMAND_ARGS_MAX];
    char       scratch[1024];

    uint32_t argc = mc_command_parse(input, name, MC_COMMAND_NAME_MAX,
                                     args, MC_COMMAND_ARGS_MAX,
                                     scratch, sizeof(scratch));

    if (name[0] == '\0') {
        snprintf(out_response, max_response, "Invalid command format. Use /command [args]");
        return MC_ERR_INVALID_ARG;
    }

    for (uint32_t i = 0; i < s_count; i++) {
        if (strcmp(s_commands[i].name, name) == 0) {
            return s_commands[i].handler(args, argc, out_response, max_response);
        }
    }

    snprintf(out_response, max_response, "Unknown command: /%s", name);
    return MC_ERR_NOT_FOUND;
}

uint32_t mc_command_complete(const char* partial, const char** out_completions, uint32_t max_completions)
{
    if (partial == NULL || out_completions == NULL || max_completions == 0) {
        return 0;
    }

    /* Skip leading '/' if present. */
    const char* prefix = partial;
    if (*prefix == '/') {
        prefix++;
    }

    uint32_t prefix_len = (uint32_t)strlen(prefix);
    uint32_t found = 0;

    for (uint32_t i = 0; i < s_count && found < max_completions; i++) {
        if (strncmp(s_commands[i].name, prefix, prefix_len) == 0) {
            out_completions[found++] = s_commands[i].name;
        }
    }

    return found;
}

uint32_t mc_command_count(void)
{
    return s_count;
}
