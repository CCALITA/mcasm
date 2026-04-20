#include "mc_memory.h"
#include <stdlib.h>

struct mc_arena { size_t cap; size_t used; char* buf; };
struct mc_pool { size_t elem; uint32_t max; uint32_t count; };

mc_error_t mc_memory_init(void) { return MC_OK; }
void mc_memory_shutdown(void) {}

mc_arena_t* mc_memory_arena_create(size_t capacity) {
    mc_arena_t* a = calloc(1, sizeof(*a));
    a->cap = capacity; a->buf = malloc(capacity); a->used = 0;
    return a;
}
void* mc_memory_arena_alloc(mc_arena_t* arena, size_t size, size_t alignment) {
    size_t aligned = (arena->used + alignment - 1) & ~(alignment - 1);
    if (aligned + size > arena->cap) return NULL;
    void* ptr = arena->buf + aligned;
    arena->used = aligned + size;
    return ptr;
}
void mc_memory_arena_reset(mc_arena_t* arena) { arena->used = 0; }
void mc_memory_arena_destroy(mc_arena_t* arena) { free(arena->buf); free(arena); }
size_t mc_memory_arena_used(const mc_arena_t* arena) { return arena->used; }

mc_pool_t* mc_memory_pool_create(size_t element_size, uint32_t max_elements) {
    mc_pool_t* p = calloc(1, sizeof(*p));
    p->elem = element_size; p->max = max_elements;
    return p;
}
void* mc_memory_pool_alloc(mc_pool_t* pool) { pool->count++; return calloc(1, pool->elem); }
void mc_memory_pool_free(mc_pool_t* pool, void* ptr) { pool->count--; free(ptr); }
void mc_memory_pool_destroy(mc_pool_t* pool) { free(pool); }
uint32_t mc_memory_pool_count(const mc_pool_t* pool) { return pool->count; }
