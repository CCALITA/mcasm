/*
 * test_entity.c — Unit tests for the mc_entity ECS module.
 *
 * Covers: create / destroy / free-list recycling, alive checks,
 *         mask queries, count, component access, set position/velocity,
 *         and foreach iteration with mask filtering.
 */

#include "mc_entity.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static int foreach_count;
static entity_id_t foreach_ids[MAX_ENTITIES];

static void count_callback(entity_id_t id, void *userdata)
{
    (void)userdata;
    foreach_ids[foreach_count++] = id;
}

static void reset_foreach(void)
{
    foreach_count = 0;
    memset(foreach_ids, 0, sizeof(foreach_ids));
}

static int float_eq(float a, float b)
{
    return fabsf(a - b) < 1e-6f;
}

/* ------------------------------------------------------------------ */
/* Tests                                                               */
/* ------------------------------------------------------------------ */

static void test_init(void)
{
    mc_error_t err = mc_entity_init();
    assert(err == MC_OK);
    assert(mc_entity_count() == 0);
    printf("  PASS test_init\n");
}

static void test_create_returns_unique_ids(void)
{
    mc_entity_init();

    entity_id_t a = mc_entity_create(COMPONENT_TRANSFORM);
    entity_id_t b = mc_entity_create(COMPONENT_TRANSFORM | COMPONENT_PHYSICS);
    entity_id_t c = mc_entity_create(COMPONENT_HEALTH);

    assert(a != ENTITY_INVALID);
    assert(b != ENTITY_INVALID);
    assert(c != ENTITY_INVALID);
    assert(a != b);
    assert(b != c);
    assert(a != c);
    assert(mc_entity_count() == 3);

    printf("  PASS test_create_returns_unique_ids\n");
}

static void test_alive(void)
{
    mc_entity_init();

    entity_id_t id = mc_entity_create(COMPONENT_TRANSFORM);
    assert(mc_entity_alive(id) == 1);
    assert(mc_entity_alive(ENTITY_INVALID) == 0);
    assert(mc_entity_alive(MAX_ENTITIES) == 0);
    assert(mc_entity_alive(MAX_ENTITIES + 100) == 0);

    printf("  PASS test_alive\n");
}

static void test_destroy(void)
{
    mc_entity_init();

    entity_id_t id = mc_entity_create(COMPONENT_TRANSFORM);
    assert(mc_entity_alive(id) == 1);
    assert(mc_entity_count() == 1);

    mc_entity_destroy(id);
    assert(mc_entity_alive(id) == 0);
    assert(mc_entity_count() == 0);

    /* Destroying an already-dead entity should be harmless */
    mc_entity_destroy(id);
    assert(mc_entity_count() == 0);

    /* Destroying invalid IDs should be harmless */
    mc_entity_destroy(ENTITY_INVALID);
    mc_entity_destroy(MAX_ENTITIES + 1);

    printf("  PASS test_destroy\n");
}

static void test_free_list_recycling(void)
{
    mc_entity_init();

    entity_id_t a = mc_entity_create(COMPONENT_TRANSFORM);
    entity_id_t b = mc_entity_create(COMPONENT_PHYSICS);

    mc_entity_destroy(a);
    assert(mc_entity_count() == 1);

    /* Next create should recycle 'a's slot */
    entity_id_t c = mc_entity_create(COMPONENT_HEALTH);
    assert(c == a);  /* reused from free list */
    assert(mc_entity_alive(c) == 1);
    assert(mc_entity_count() == 2);

    /* Verify masks are set correctly for reused ID */
    assert(mc_entity_get_mask(c) == COMPONENT_HEALTH);
    assert(mc_entity_get_mask(b) == COMPONENT_PHYSICS);

    printf("  PASS test_free_list_recycling\n");
}

static void test_get_mask(void)
{
    mc_entity_init();

    uint32_t mask = COMPONENT_TRANSFORM | COMPONENT_HEALTH | COMPONENT_AI;
    entity_id_t id = mc_entity_create(mask);
    assert(mc_entity_get_mask(id) == mask);

    /* Invalid ID returns 0 */
    assert(mc_entity_get_mask(ENTITY_INVALID) == 0);

    printf("  PASS test_get_mask\n");
}

static void test_get_transform(void)
{
    mc_entity_init();

    entity_id_t id = mc_entity_create(COMPONENT_TRANSFORM);
    transform_component_t *t = mc_entity_get_transform(id);
    assert(t != NULL);

    /* Different entities should get different pointers */
    entity_id_t id2 = mc_entity_create(COMPONENT_TRANSFORM);
    transform_component_t *t2 = mc_entity_get_transform(id2);
    assert(t2 != NULL);
    assert(t != t2);

    printf("  PASS test_get_transform\n");
}

static void test_get_physics(void)
{
    mc_entity_init();

    entity_id_t id = mc_entity_create(COMPONENT_PHYSICS);
    physics_component_t *p = mc_entity_get_physics(id);
    assert(p != NULL);

    printf("  PASS test_get_physics\n");
}

static void test_get_health(void)
{
    mc_entity_init();

    entity_id_t id = mc_entity_create(COMPONENT_HEALTH);
    health_component_t *h = mc_entity_get_health(id);
    assert(h != NULL);

    printf("  PASS test_get_health\n");
}

static void test_set_position(void)
{
    mc_entity_init();

    entity_id_t id = mc_entity_create(COMPONENT_TRANSFORM);
    vec3_t pos = { .x = 1.0f, .y = 2.0f, .z = 3.0f, ._pad = 0.0f };
    mc_entity_set_position(id, pos);

    transform_component_t *t = mc_entity_get_transform(id);
    assert(float_eq(t->position.x, 1.0f));
    assert(float_eq(t->position.y, 2.0f));
    assert(float_eq(t->position.z, 3.0f));

    printf("  PASS test_set_position\n");
}

static void test_set_velocity(void)
{
    mc_entity_init();

    entity_id_t id = mc_entity_create(COMPONENT_TRANSFORM);
    vec3_t vel = { .x = 0.5f, .y = -9.8f, .z = 0.0f, ._pad = 0.0f };
    mc_entity_set_velocity(id, vel);

    transform_component_t *t = mc_entity_get_transform(id);
    assert(float_eq(t->velocity.x, 0.5f));
    assert(float_eq(t->velocity.y, -9.8f));
    assert(float_eq(t->velocity.z, 0.0f));

    printf("  PASS test_set_velocity\n");
}

static void test_foreach_all(void)
{
    mc_entity_init();

    entity_id_t a = mc_entity_create(COMPONENT_TRANSFORM);
    entity_id_t b = mc_entity_create(COMPONENT_TRANSFORM | COMPONENT_PHYSICS);
    entity_id_t c = mc_entity_create(COMPONENT_PHYSICS);

    reset_foreach();
    mc_entity_foreach(COMPONENT_TRANSFORM, count_callback, NULL);
    assert(foreach_count == 2);

    /* Both a and b should be visited (they have TRANSFORM) */
    int found_a = 0, found_b = 0;
    for (int i = 0; i < foreach_count; i++) {
        if (foreach_ids[i] == a) found_a = 1;
        if (foreach_ids[i] == b) found_b = 1;
    }
    assert(found_a && found_b);

    /* c does not have TRANSFORM, so shouldn't appear */
    (void)c;

    printf("  PASS test_foreach_all\n");
}

static void test_foreach_combined_mask(void)
{
    mc_entity_init();

    mc_entity_create(COMPONENT_TRANSFORM);
    entity_id_t b = mc_entity_create(COMPONENT_TRANSFORM | COMPONENT_PHYSICS);
    mc_entity_create(COMPONENT_PHYSICS);

    reset_foreach();
    mc_entity_foreach(COMPONENT_TRANSFORM | COMPONENT_PHYSICS, count_callback, NULL);
    assert(foreach_count == 1);
    assert(foreach_ids[0] == b);

    printf("  PASS test_foreach_combined_mask\n");
}

static void test_foreach_empty(void)
{
    mc_entity_init();

    reset_foreach();
    mc_entity_foreach(COMPONENT_TRANSFORM, count_callback, NULL);
    assert(foreach_count == 0);

    printf("  PASS test_foreach_empty\n");
}

static void test_many_create_destroy(void)
{
    mc_entity_init();

    /* Create many entities */
    entity_id_t ids[100];
    for (int i = 0; i < 100; i++) {
        ids[i] = mc_entity_create(COMPONENT_TRANSFORM);
        assert(ids[i] != ENTITY_INVALID);
    }
    assert(mc_entity_count() == 100);

    /* Destroy every other one */
    for (int i = 0; i < 100; i += 2) {
        mc_entity_destroy(ids[i]);
    }
    assert(mc_entity_count() == 50);

    /* Recreate — should reuse freed slots */
    for (int i = 0; i < 50; i++) {
        entity_id_t id = mc_entity_create(COMPONENT_PHYSICS);
        assert(id != ENTITY_INVALID);
        assert(mc_entity_alive(id));
    }
    assert(mc_entity_count() == 100);

    printf("  PASS test_many_create_destroy\n");
}

static void test_capacity_limit(void)
{
    mc_entity_init();

    /* Fill up to MAX_ENTITIES - 1 (slot 0 is ENTITY_INVALID) */
    for (uint32_t i = 1; i < MAX_ENTITIES; i++) {
        entity_id_t id = mc_entity_create(COMPONENT_TRANSFORM);
        assert(id != ENTITY_INVALID);
    }
    assert(mc_entity_count() == MAX_ENTITIES - 1);

    /* Next create should fail */
    entity_id_t overflow = mc_entity_create(COMPONENT_TRANSFORM);
    assert(overflow == ENTITY_INVALID);

    /* Destroy one and create again — should succeed via free list */
    mc_entity_destroy(42);
    entity_id_t recycled = mc_entity_create(COMPONENT_PHYSICS);
    assert(recycled == 42);
    assert(mc_entity_get_mask(recycled) == COMPONENT_PHYSICS);

    printf("  PASS test_capacity_limit\n");
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    printf("mc_entity tests:\n");

    test_init();
    test_create_returns_unique_ids();
    test_alive();
    test_destroy();
    test_free_list_recycling();
    test_get_mask();
    test_get_transform();
    test_get_physics();
    test_get_health();
    test_set_position();
    test_set_velocity();
    test_foreach_all();
    test_foreach_combined_mask();
    test_foreach_empty();
    test_many_create_destroy();
    test_capacity_limit();

    printf("ALL PASSED\n");
    return 0;
}
