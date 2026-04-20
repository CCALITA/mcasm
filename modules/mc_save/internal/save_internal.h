#ifndef MC_SAVE_INTERNAL_H
#define MC_SAVE_INTERNAL_H

#include "mc_error.h"
#include <stdint.h>
#include <stddef.h>

/* Maximum path length for save files. */
#define SAVE_MAX_PATH 512

/* Region file constants (Anvil format). */
#define REGION_CHUNKS_X       32
#define REGION_CHUNKS_Z       32
#define REGION_SECTOR_SIZE    4096
#define REGION_HEADER_SECTORS 2       /* offset table + timestamp table */
#define REGION_HEADER_SIZE    (REGION_HEADER_SECTORS * REGION_SECTOR_SIZE)

/* Global state: base directory for the current world. */
extern char g_world_dir[SAVE_MAX_PATH];

/* Utility: ensure a directory exists (recursive). */
mc_error_t save_ensure_dir(const char* path);

/* Utility: build a path into buf. Returns MC_ERR_IO if too long. */
mc_error_t save_build_path(char* buf, size_t buf_size, const char* fmt, ...);

/* Floor-division that rounds toward negative infinity. */
static inline int32_t floor_div(int32_t a, int32_t b)
{
    return (a / b) - (a % b != 0 && (a ^ b) < 0);
}

#endif /* MC_SAVE_INTERNAL_H */
