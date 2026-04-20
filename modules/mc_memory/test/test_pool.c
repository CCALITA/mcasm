#include "mc_memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define TEST(name) static void name(void)
#define RUN(name) do { printf("  %s... ", #name); name(); printf("OK\n"); } while(0)

TEST(test_pool_create_destroy) {
    mc_pool_t *p = mc_memory_pool_create(64, 16);
    assert(p != NULL);
    assert(mc_memory_pool_count(p) == 0);
    mc_memory_pool_destroy(p);
}

TEST(test_pool_alloc_single) {
    mc_pool_t *p = mc_memory_pool_create(64, 16);
    void *elem = mc_memory_pool_alloc(p);
    assert(elem != NULL);
    assert(mc_memory_pool_count(p) == 1);
    mc_memory_pool_destroy(p);
}

TEST(test_pool_alloc_multiple) {
    mc_pool_t *p = mc_memory_pool_create(64, 8);
    void *elems[8];
    for (int i = 0; i < 8; i++) {
        elems[i] = mc_memory_pool_alloc(p);
        assert(elems[i] != NULL);
    }
    assert(mc_memory_pool_count(p) == 8);

    /* Verify all pointers are distinct */
    for (int i = 0; i < 8; i++) {
        for (int j = i + 1; j < 8; j++) {
            assert(elems[i] != elems[j]);
        }
    }
    mc_memory_pool_destroy(p);
}

TEST(test_pool_free) {
    mc_pool_t *p = mc_memory_pool_create(64, 16);
    void *a = mc_memory_pool_alloc(p);
    void *b = mc_memory_pool_alloc(p);
    assert(mc_memory_pool_count(p) == 2);

    mc_memory_pool_free(p, a);
    assert(mc_memory_pool_count(p) == 1);

    mc_memory_pool_free(p, b);
    assert(mc_memory_pool_count(p) == 0);

    mc_memory_pool_destroy(p);
}

TEST(test_pool_alloc_after_free) {
    mc_pool_t *p = mc_memory_pool_create(64, 4);
    void *a = mc_memory_pool_alloc(p);
    (void)mc_memory_pool_alloc(p);

    mc_memory_pool_free(p, a);
    assert(mc_memory_pool_count(p) == 1);

    /* Should reuse the freed slot */
    void *c = mc_memory_pool_alloc(p);
    assert(c != NULL);
    assert(mc_memory_pool_count(p) == 2);

    /* The recycled pointer should be the one we freed */
    assert(c == a);

    mc_memory_pool_destroy(p);
}

TEST(test_pool_exhaust) {
    mc_pool_t *p = mc_memory_pool_create(32, 4);
    for (int i = 0; i < 4; i++) {
        void *elem = mc_memory_pool_alloc(p);
        assert(elem != NULL);
    }
    assert(mc_memory_pool_count(p) == 4);

    /* Pool should be exhausted — next alloc returns NULL */
    void *over = mc_memory_pool_alloc(p);
    assert(over == NULL);
    assert(mc_memory_pool_count(p) == 4);

    mc_memory_pool_destroy(p);
}

TEST(test_pool_write_read) {
    /* Use a pool of uint64_t-sized elements and verify read-back */
    mc_pool_t *p = mc_memory_pool_create(sizeof(uint64_t), 8);

    uint64_t *a = mc_memory_pool_alloc(p);
    uint64_t *b = mc_memory_pool_alloc(p);
    assert(a && b);

    *a = 0xDEADBEEF;
    *b = 0xCAFEBABE;

    assert(*a == 0xDEADBEEF);
    assert(*b == 0xCAFEBABE);

    mc_memory_pool_destroy(p);
}

TEST(test_pool_small_element) {
    /* Element size smaller than a pointer — should be bumped to 8 internally */
    mc_pool_t *p = mc_memory_pool_create(1, 4);
    assert(p != NULL);

    void *a = mc_memory_pool_alloc(p);
    void *b = mc_memory_pool_alloc(p);
    assert(a != NULL && b != NULL);
    assert(a != b);

    mc_memory_pool_free(p, a);
    void *c = mc_memory_pool_alloc(p);
    assert(c == a);

    mc_memory_pool_destroy(p);
}

int main(void) {
    printf("test_pool:\n");
    RUN(test_pool_create_destroy);
    RUN(test_pool_alloc_single);
    RUN(test_pool_alloc_multiple);
    RUN(test_pool_free);
    RUN(test_pool_alloc_after_free);
    RUN(test_pool_exhaust);
    RUN(test_pool_write_read);
    RUN(test_pool_small_element);
    printf("  ALL PASSED\n");
    return 0;
}
