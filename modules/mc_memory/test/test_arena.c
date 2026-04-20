#include "mc_memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("OK\n"); } while(0)

TEST(test_arena_create_destroy) {
    mc_arena_t *a = mc_memory_arena_create(4096);
    assert(a != NULL);
    assert(mc_memory_arena_used(a) == 0);
    mc_memory_arena_destroy(a);
}

TEST(test_arena_alloc_basic) {
    mc_arena_t *a = mc_memory_arena_create(4096);
    void *p = mc_memory_arena_alloc(a, 64, 1);
    assert(p != NULL);
    assert(mc_memory_arena_used(a) == 64);
    mc_memory_arena_destroy(a);
}

TEST(test_arena_alloc_alignment) {
    mc_arena_t *a = mc_memory_arena_create(4096);

    /* Allocate 1 byte, then request 16-byte aligned allocation. */
    void *p1 = mc_memory_arena_alloc(a, 1, 1);
    assert(p1 != NULL);

    void *p2 = mc_memory_arena_alloc(a, 32, 16);
    assert(p2 != NULL);
    assert(((uintptr_t)p2 & 15) == 0);  /* must be 16-byte aligned */

    /* The two allocations must not overlap */
    assert((char *)p2 >= (char *)p1 + 1);

    mc_memory_arena_destroy(a);
}

TEST(test_arena_alloc_multiple_alignments) {
    mc_arena_t *a = mc_memory_arena_create(4096);

    void *p1 = mc_memory_arena_alloc(a, 7, 1);
    assert(p1 != NULL);

    void *p2 = mc_memory_arena_alloc(a, 16, 8);
    assert(p2 != NULL);
    assert(((uintptr_t)p2 & 7) == 0);

    void *p3 = mc_memory_arena_alloc(a, 64, 64);
    assert(p3 != NULL);
    assert(((uintptr_t)p3 & 63) == 0);

    mc_memory_arena_destroy(a);
}

TEST(test_arena_reset) {
    mc_arena_t *a = mc_memory_arena_create(4096);
    mc_memory_arena_alloc(a, 100, 1);
    assert(mc_memory_arena_used(a) == 100);

    mc_memory_arena_reset(a);
    assert(mc_memory_arena_used(a) == 0);

    /* Can allocate again after reset */
    void *p = mc_memory_arena_alloc(a, 200, 1);
    assert(p != NULL);
    assert(mc_memory_arena_used(a) == 200);

    mc_memory_arena_destroy(a);
}

TEST(test_arena_exhaust) {
    mc_arena_t *a = mc_memory_arena_create(128);
    void *p1 = mc_memory_arena_alloc(a, 100, 1);
    assert(p1 != NULL);

    /* This should fail — only 28 bytes left */
    void *p2 = mc_memory_arena_alloc(a, 100, 1);
    assert(p2 == NULL);

    mc_memory_arena_destroy(a);
}

TEST(test_arena_exact_fit) {
    mc_arena_t *a = mc_memory_arena_create(64);
    void *p = mc_memory_arena_alloc(a, 64, 1);
    assert(p != NULL);
    assert(mc_memory_arena_used(a) == 64);

    /* No more room */
    void *p2 = mc_memory_arena_alloc(a, 1, 1);
    assert(p2 == NULL);

    mc_memory_arena_destroy(a);
}

TEST(test_arena_sequential_writes) {
    mc_arena_t *a = mc_memory_arena_create(4096);
    /* Allocate several blocks and write to them to verify no overlaps */
    int *a1 = mc_memory_arena_alloc(a, sizeof(int), 4);
    int *a2 = mc_memory_arena_alloc(a, sizeof(int), 4);
    int *a3 = mc_memory_arena_alloc(a, sizeof(int), 4);
    assert(a1 && a2 && a3);

    *a1 = 111;
    *a2 = 222;
    *a3 = 333;

    assert(*a1 == 111);
    assert(*a2 == 222);
    assert(*a3 == 333);

    mc_memory_arena_destroy(a);
}

int main(void) {
    printf("test_arena:\n");
    RUN(test_arena_create_destroy);
    RUN(test_arena_alloc_basic);
    RUN(test_arena_alloc_alignment);
    RUN(test_arena_alloc_multiple_alignments);
    RUN(test_arena_reset);
    RUN(test_arena_exhaust);
    RUN(test_arena_exact_fit);
    RUN(test_arena_sequential_writes);
    printf("  ALL PASSED\n");
    return 0;
}
