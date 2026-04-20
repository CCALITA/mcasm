#ifndef MC_MEMORY_H
#define MC_MEMORY_H

#include "mc_types.h"
#include "mc_error.h"

typedef struct mc_arena mc_arena_t;
typedef struct mc_pool mc_pool_t;

mc_error_t  mc_memory_init(void);
void        mc_memory_shutdown(void);

mc_arena_t* mc_memory_arena_create(size_t capacity);
void*       mc_memory_arena_alloc(mc_arena_t* arena, size_t size, size_t alignment);
void        mc_memory_arena_reset(mc_arena_t* arena);
void        mc_memory_arena_destroy(mc_arena_t* arena);
size_t      mc_memory_arena_used(const mc_arena_t* arena);

mc_pool_t*  mc_memory_pool_create(size_t element_size, uint32_t max_elements);
void*       mc_memory_pool_alloc(mc_pool_t* pool);
void        mc_memory_pool_free(mc_pool_t* pool, void* ptr);
void        mc_memory_pool_destroy(mc_pool_t* pool);
uint32_t    mc_memory_pool_count(const mc_pool_t* pool);

#endif /* MC_MEMORY_H */
