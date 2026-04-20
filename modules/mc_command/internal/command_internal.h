#ifndef MC_COMMAND_INTERNAL_H
#define MC_COMMAND_INTERNAL_H

#include "mc_command.h"

#define MC_COMMAND_MAX        64
#define MC_COMMAND_NAME_MAX   32
#define MC_COMMAND_USAGE_MAX  128
#define MC_COMMAND_ARGS_MAX   16

typedef struct {
    char          name[MC_COMMAND_NAME_MAX];
    char          usage[MC_COMMAND_USAGE_MAX];
    mc_command_fn handler;
} mc_command_entry_t;

/* Parser: split input into command name and arguments.
 * Returns the number of arguments parsed (excluding the command name).
 * out_name receives the command name (without the leading '/').
 * out_args receives pointers into the scratch buffer.
 * scratch is a mutable copy of the input that the parser tokenizes in-place. */
uint32_t mc_command_parse(const char* input,
                          char* out_name, uint32_t name_max,
                          const char** out_args, uint32_t max_args,
                          char* scratch, uint32_t scratch_max);

/* Register the built-in default commands. Called from mc_command_init(). */
mc_error_t mc_command_register_defaults(void);

/* Access to the registry (used by defaults.c for /help) */
const mc_command_entry_t* mc_command_get_registry(void);

#endif /* MC_COMMAND_INTERNAL_H */
