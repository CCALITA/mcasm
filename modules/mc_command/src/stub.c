#include "mc_command.h"
#include <string.h>
mc_error_t mc_command_init(void) { return MC_OK; }
void mc_command_shutdown(void) {}
mc_error_t mc_command_register(const char* n, const char* u, mc_command_fn h) { (void)n;(void)u;(void)h; return MC_OK; }
mc_error_t mc_command_execute(const char* i, char* o, uint32_t max) { (void)i; if(o&&max>0) o[0]=0; return MC_ERR_NOT_FOUND; }
uint32_t mc_command_complete(const char* p, const char** o, uint32_t max) { (void)p;(void)o;(void)max; return 0; }
uint32_t mc_command_count(void) { return 0; }
