/*
 * arena_fallback.c -- Pure-C fallback for arena.asm
 *
 * Used on non-x86-64 architectures (e.g. Apple Silicon) where NASM
 * cannot produce native code.  Implements the same arena (bump)
 * allocator logic as arena.asm.
 */

#include "mc_memory.h"
#include <stdlib.h>
#include <string.h>

#ifndef __x86_64__

struct mc_arena {
    size_t cap;
    size_t used;
    char  *buf;
};

mc_arena_t *mc_memory_arena_create(size_t capacity)
{
    mc_arena_t *arena = calloc(1, sizeof(mc_arena_t));
    if (!arena) return NULL;

    arena->buf = malloc(capacity);
    if (!arena->buf) {
        free(arena);
        return NULL;
    }

    arena->cap  = capacity;
    arena->used = 0;
    return arena;
}

void *mc_memory_arena_alloc(mc_arena_t *arena, size_t size, size_t alignment)
{
    size_t aligned = (arena->used + alignment - 1) & ~(alignment - 1);

    if (aligned + size > arena->cap) return NULL;

    arena->used = aligned + size;
    return arena->buf + aligned;
}

void mc_memory_arena_reset(mc_arena_t *arena)
{
    arena->used = 0;
}

void mc_memory_arena_destroy(mc_arena_t *arena)
{
    free(arena->buf);
    free(arena);
}

size_t mc_memory_arena_used(const mc_arena_t *arena)
{
    return arena->used;
}

#endif /* !__x86_64__ */
