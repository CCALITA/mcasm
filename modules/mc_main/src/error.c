#include "mc_error.h"

const char *mc_error_string(mc_error_t err)
{
    switch (err) {
    case MC_OK:                return "OK";
    case MC_ERR_INVALID_ARG:   return "invalid argument";
    case MC_ERR_OUT_OF_MEMORY: return "out of memory";
    case MC_ERR_NOT_FOUND:     return "not found";
    case MC_ERR_ALREADY_EXISTS:return "already exists";
    case MC_ERR_IO:            return "I/O error";
    case MC_ERR_VULKAN:        return "Vulkan error";
    case MC_ERR_PLATFORM:      return "platform error";
    case MC_ERR_FULL:          return "full";
    case MC_ERR_EMPTY:         return "empty";
    case MC_ERR_NOT_READY:     return "not ready";
    case MC_ERR_NETWORK:       return "network error";
    case MC_ERR_PROTOCOL:      return "protocol error";
    default:                   return "unknown error";
    }
}
