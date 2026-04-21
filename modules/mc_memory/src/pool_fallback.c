/*
 * pool_fallback.c -- Pure-C fallback for pool.asm
 *
 * Used on non-x86-64 architectures (e.g. Apple Silicon) where NASM
 * cannot produce native code.  Implements the same pool (freelist)
 * allocator logic as pool.asm.
 */

#include "mc_memory.h"
#include <stdlib.h>
#include <string.h>

#ifndef __x86_64__

struct mc_pool {
    size_t   elem_size;
    uint32_t max_elements;
    uint32_t count;
    void    *backing;
    void    *freelist;
};

mc_pool_t *mc_memory_pool_create(size_t element_size, uint32_t max_elements)
{
    if (element_size < sizeof(void *))
        element_size = sizeof(void *);

    mc_pool_t *pool = calloc(1, sizeof(mc_pool_t));
    if (!pool) return NULL;

    pool->backing = malloc(element_size * max_elements);
    if (!pool->backing) {
        free(pool);
        return NULL;
    }

    pool->elem_size    = element_size;
    pool->max_elements = max_elements;
    pool->count        = 0;

    /* Build intrusive freelist: each element points to the next. */
    char *base = pool->backing;
    for (uint32_t i = 0; i < max_elements - 1; i++) {
        void **slot = (void **)(base + i * element_size);
        *slot = base + (i + 1) * element_size;
    }
    /* Last element points to NULL. */
    void **last = (void **)(base + (max_elements - 1) * element_size);
    *last = NULL;

    pool->freelist = pool->backing;
    return pool;
}

void *mc_memory_pool_alloc(mc_pool_t *pool)
{
    if (!pool->freelist) return NULL;

    void *head = pool->freelist;
    pool->freelist = *(void **)head;
    pool->count++;
    return head;
}

void mc_memory_pool_free(mc_pool_t *pool, void *ptr)
{
    *(void **)ptr = pool->freelist;
    pool->freelist = ptr;
    pool->count--;
}

void mc_memory_pool_destroy(mc_pool_t *pool)
{
    free(pool->backing);
    free(pool);
}

uint32_t mc_memory_pool_count(const mc_pool_t *pool)
{
    return pool->count;
}

#endif /* !__x86_64__ */
