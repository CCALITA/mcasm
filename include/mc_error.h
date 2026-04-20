#ifndef MC_ERROR_H
#define MC_ERROR_H

#include <stdint.h>

typedef int32_t mc_error_t;

#define MC_OK                  0
#define MC_ERR_INVALID_ARG     1
#define MC_ERR_OUT_OF_MEMORY   2
#define MC_ERR_NOT_FOUND       3
#define MC_ERR_ALREADY_EXISTS  4
#define MC_ERR_IO              5
#define MC_ERR_VULKAN          6
#define MC_ERR_PLATFORM        7
#define MC_ERR_FULL            8
#define MC_ERR_EMPTY           9
#define MC_ERR_NOT_READY       10
#define MC_ERR_NETWORK         11
#define MC_ERR_PROTOCOL        12

const char* mc_error_string(mc_error_t err);

#endif /* MC_ERROR_H */
