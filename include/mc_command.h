#ifndef MC_COMMAND_H
#define MC_COMMAND_H

#include "mc_types.h"
#include "mc_error.h"

typedef mc_error_t (*mc_command_fn)(const char** args, uint32_t argc, char* out_response, uint32_t max_response);

mc_error_t mc_command_init(void);
void       mc_command_shutdown(void);

mc_error_t mc_command_register(const char* name, const char* usage, mc_command_fn handler);
mc_error_t mc_command_execute(const char* input, char* out_response, uint32_t max_response);
uint32_t   mc_command_complete(const char* partial, const char** out_completions, uint32_t max_completions);
uint32_t   mc_command_count(void);

#endif /* MC_COMMAND_H */
