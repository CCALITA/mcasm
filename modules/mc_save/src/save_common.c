/*
 * save_common.c -- Shared state and utilities for the mc_save module.
 */

#include "save_internal.h"
#include "mc_save.h"
#include "mc_error.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>

/* Module-global world directory. */
char g_world_dir[SAVE_MAX_PATH] = {0};

/* ------------------------------------------------------------------ */
/*  mc_save_init / shutdown                                            */
/* ------------------------------------------------------------------ */

mc_error_t mc_save_init(const char* world_dir)
{
    if (!world_dir) return MC_ERR_INVALID_ARG;
    if (strlen(world_dir) >= SAVE_MAX_PATH - 32) return MC_ERR_INVALID_ARG;

    strncpy(g_world_dir, world_dir, SAVE_MAX_PATH - 1);
    g_world_dir[SAVE_MAX_PATH - 1] = '\0';

    /* Strip trailing slash. */
    size_t len = strlen(g_world_dir);
    while (len > 0 && g_world_dir[len - 1] == '/')
        g_world_dir[--len] = '\0';

    /* Create directory tree. */
    mc_error_t err;
    char path[SAVE_MAX_PATH];

    err = save_ensure_dir(g_world_dir);
    if (err) return err;

    err = save_build_path(path, sizeof(path), "%s/region", g_world_dir);
    if (err) return err;
    err = save_ensure_dir(path);
    if (err) return err;

    err = save_build_path(path, sizeof(path), "%s/players", g_world_dir);
    if (err) return err;
    err = save_ensure_dir(path);
    if (err) return err;

    return MC_OK;
}

void mc_save_shutdown(void)
{
    g_world_dir[0] = '\0';
}

/* ------------------------------------------------------------------ */
/*  Utility functions                                                  */
/* ------------------------------------------------------------------ */

mc_error_t save_ensure_dir(const char* path)
{
    if (!path) return MC_ERR_INVALID_ARG;

    /* Try mkdir first; succeed if it already exists. */
    if (mkdir(path, 0755) == 0) return MC_OK;
    if (errno == EEXIST) return MC_OK;

    /* Walk the path and create each component. */
    char tmp[SAVE_MAX_PATH];
    strncpy(tmp, path, SAVE_MAX_PATH - 1);
    tmp[SAVE_MAX_PATH - 1] = '\0';

    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
                return MC_ERR_IO;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
        return MC_ERR_IO;

    return MC_OK;
}

mc_error_t save_build_path(char* buf, size_t buf_size, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, buf_size, fmt, ap);
    va_end(ap);

    if (n < 0 || (size_t)n >= buf_size) return MC_ERR_IO;
    return MC_OK;
}
