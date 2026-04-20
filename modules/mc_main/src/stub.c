#include "mc_types.h"
#include "mc_error.h"

const char* mc_error_string(mc_error_t err) {
    switch(err) {
        case MC_OK: return "OK";
        case MC_ERR_INVALID_ARG: return "invalid argument";
        case MC_ERR_OUT_OF_MEMORY: return "out of memory";
        case MC_ERR_NOT_FOUND: return "not found";
        default: return "unknown error";
    }
}

int main(void) {
    return 0;
}
