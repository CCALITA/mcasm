/*
 * test_inventory.c — Unit tests for the mc_inventory API.
 *
 * Covers: add/remove items, stacking, max stack size, hotbar selection,
 *         boundary conditions, and error handling.
 */

#include "mc_inventory.h"
#include "mc_entity.h"
#include "mc_block.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/*
 * Create a fresh entity with COMPONENT_INVENTORY and zero its inventory.
 */
static entity_id_t fresh_entity(void)
{
    mc_entity_init();
    entity_id_t id = mc_entity_create(COMPONENT_INVENTORY);
    assert(id != ENTITY_INVALID);

    inventory_t *inv = mc_inventory_get(id);
    assert(inv != NULL);
    memset(inv, 0, sizeof(*inv));

    return id;
}

/* ------------------------------------------------------------------ */
/* Tests                                                               */
/* ------------------------------------------------------------------ */

static void test_get_valid(void)
{
    entity_id_t id = fresh_entity();
    inventory_t *inv = mc_inventory_get(id);
    assert(inv != NULL);
    printf("  PASS test_get_valid\n");
}

static void test_get_invalid(void)
{
    assert(mc_inventory_get(ENTITY_INVALID) == NULL);
    assert(mc_inventory_get(MAX_ENTITIES) == NULL);
    assert(mc_inventory_get(MAX_ENTITIES + 1) == NULL);
    printf("  PASS test_get_invalid\n");
}

static void test_add_single_item(void)
{
    entity_id_t id = fresh_entity();
    mc_error_t err = mc_inventory_add_item(id, BLOCK_STONE, 10);
    assert(err == MC_OK);

    inventory_t *inv = mc_inventory_get(id);
    assert(inv->slots[0].id == BLOCK_STONE);
    assert(inv->slots[0].count == 10);

    printf("  PASS test_add_single_item\n");
}

static void test_add_stacks_same_type(void)
{
    entity_id_t id = fresh_entity();

    mc_inventory_add_item(id, BLOCK_DIRT, 30);
    mc_error_t err = mc_inventory_add_item(id, BLOCK_DIRT, 20);
    assert(err == MC_OK);

    inventory_t *inv = mc_inventory_get(id);
    /* 30 + 20 = 50, fits in one stack (<= 64) */
    assert(inv->slots[0].id == BLOCK_DIRT);
    assert(inv->slots[0].count == 50);
    /* Second slot should still be empty */
    assert(inv->slots[1].count == 0);

    printf("  PASS test_add_stacks_same_type\n");
}

static void test_add_overflow_stack(void)
{
    entity_id_t id = fresh_entity();

    mc_inventory_add_item(id, BLOCK_COBBLESTONE, 60);
    mc_error_t err = mc_inventory_add_item(id, BLOCK_COBBLESTONE, 10);
    assert(err == MC_OK);

    inventory_t *inv = mc_inventory_get(id);
    /* First slot capped at 64, remainder (6) spills to second slot */
    assert(inv->slots[0].id == BLOCK_COBBLESTONE);
    assert(inv->slots[0].count == 64);
    assert(inv->slots[1].id == BLOCK_COBBLESTONE);
    assert(inv->slots[1].count == 6);

    printf("  PASS test_add_overflow_stack\n");
}

static void test_add_different_types_no_stack(void)
{
    entity_id_t id = fresh_entity();

    mc_inventory_add_item(id, BLOCK_STONE, 10);
    mc_inventory_add_item(id, BLOCK_DIRT, 5);

    inventory_t *inv = mc_inventory_get(id);
    assert(inv->slots[0].id == BLOCK_STONE);
    assert(inv->slots[0].count == 10);
    assert(inv->slots[1].id == BLOCK_DIRT);
    assert(inv->slots[1].count == 5);

    printf("  PASS test_add_different_types_no_stack\n");
}

static void test_add_max_stack_size(void)
{
    entity_id_t id = fresh_entity();

    mc_error_t err = mc_inventory_add_item(id, BLOCK_SAND, 64);
    assert(err == MC_OK);

    inventory_t *inv = mc_inventory_get(id);
    assert(inv->slots[0].count == 64);

    /* Adding more should go to the next slot */
    err = mc_inventory_add_item(id, BLOCK_SAND, 1);
    assert(err == MC_OK);
    assert(inv->slots[1].id == BLOCK_SAND);
    assert(inv->slots[1].count == 1);

    printf("  PASS test_add_max_stack_size\n");
}

static void test_add_inventory_full(void)
{
    entity_id_t id = fresh_entity();

    /* Fill all 36 slots to max */
    for (int i = 0; i < INVENTORY_SIZE; ++i) {
        mc_error_t err = mc_inventory_add_item(id, BLOCK_STONE, 64);
        assert(err == MC_OK);
    }

    /* Next add should fail */
    mc_error_t err = mc_inventory_add_item(id, BLOCK_STONE, 1);
    assert(err == MC_ERR_FULL);

    printf("  PASS test_add_inventory_full\n");
}

static void test_add_zero_count(void)
{
    entity_id_t id = fresh_entity();
    mc_error_t err = mc_inventory_add_item(id, BLOCK_STONE, 0);
    assert(err == MC_ERR_INVALID_ARG);

    printf("  PASS test_add_zero_count\n");
}

static void test_add_invalid_entity(void)
{
    mc_error_t err = mc_inventory_add_item(ENTITY_INVALID, BLOCK_STONE, 1);
    assert(err == MC_ERR_INVALID_ARG);

    err = mc_inventory_add_item(MAX_ENTITIES, BLOCK_STONE, 1);
    assert(err == MC_ERR_INVALID_ARG);

    printf("  PASS test_add_invalid_entity\n");
}

static void test_remove_partial(void)
{
    entity_id_t id = fresh_entity();
    mc_inventory_add_item(id, BLOCK_STONE, 20);

    mc_error_t err = mc_inventory_remove_item(id, 0, 5);
    assert(err == MC_OK);

    inventory_t *inv = mc_inventory_get(id);
    assert(inv->slots[0].id == BLOCK_STONE);
    assert(inv->slots[0].count == 15);

    printf("  PASS test_remove_partial\n");
}

static void test_remove_all(void)
{
    entity_id_t id = fresh_entity();
    mc_inventory_add_item(id, BLOCK_STONE, 10);

    mc_error_t err = mc_inventory_remove_item(id, 0, 10);
    assert(err == MC_OK);

    inventory_t *inv = mc_inventory_get(id);
    assert(inv->slots[0].count == 0);
    assert(inv->slots[0].id == 0);

    printf("  PASS test_remove_all\n");
}

static void test_remove_more_than_present(void)
{
    entity_id_t id = fresh_entity();
    mc_inventory_add_item(id, BLOCK_DIRT, 5);

    /* Removing more than the slot holds clears the slot */
    mc_error_t err = mc_inventory_remove_item(id, 0, 100);
    assert(err == MC_OK);

    inventory_t *inv = mc_inventory_get(id);
    assert(inv->slots[0].count == 0);
    assert(inv->slots[0].id == 0);

    printf("  PASS test_remove_more_than_present\n");
}

static void test_remove_from_empty_slot(void)
{
    entity_id_t id = fresh_entity();

    mc_error_t err = mc_inventory_remove_item(id, 0, 1);
    assert(err == MC_ERR_EMPTY);

    printf("  PASS test_remove_from_empty_slot\n");
}

static void test_remove_invalid_slot(void)
{
    entity_id_t id = fresh_entity();

    mc_error_t err = mc_inventory_remove_item(id, INVENTORY_SIZE, 1);
    assert(err == MC_ERR_INVALID_ARG);

    err = mc_inventory_remove_item(id, 255, 1);
    assert(err == MC_ERR_INVALID_ARG);

    printf("  PASS test_remove_invalid_slot\n");
}

static void test_remove_invalid_entity(void)
{
    mc_error_t err = mc_inventory_remove_item(ENTITY_INVALID, 0, 1);
    assert(err == MC_ERR_INVALID_ARG);

    printf("  PASS test_remove_invalid_entity\n");
}

static void test_hotbar_selection_default(void)
{
    entity_id_t id = fresh_entity();

    inventory_t *inv = mc_inventory_get(id);
    assert(inv->selected_hotbar == 0);

    printf("  PASS test_hotbar_selection_default\n");
}

static void test_set_selected_valid(void)
{
    entity_id_t id = fresh_entity();

    for (uint8_t slot = 0; slot < HOTBAR_SLOTS; ++slot) {
        mc_error_t err = mc_inventory_set_selected(id, slot);
        assert(err == MC_OK);

        inventory_t *inv = mc_inventory_get(id);
        assert(inv->selected_hotbar == slot);
    }

    printf("  PASS test_set_selected_valid\n");
}

static void test_set_selected_out_of_range(void)
{
    entity_id_t id = fresh_entity();

    mc_error_t err = mc_inventory_set_selected(id, HOTBAR_SLOTS);
    assert(err == MC_ERR_INVALID_ARG);

    err = mc_inventory_set_selected(id, 255);
    assert(err == MC_ERR_INVALID_ARG);

    printf("  PASS test_set_selected_out_of_range\n");
}

static void test_set_selected_invalid_entity(void)
{
    mc_error_t err = mc_inventory_set_selected(ENTITY_INVALID, 0);
    assert(err == MC_ERR_INVALID_ARG);

    printf("  PASS test_set_selected_invalid_entity\n");
}

static void test_get_selected_item(void)
{
    entity_id_t id = fresh_entity();

    mc_inventory_add_item(id, BLOCK_OAK_PLANKS, 32);
    mc_inventory_set_selected(id, 0);

    item_stack_t sel = mc_inventory_get_selected(id);
    assert(sel.id == BLOCK_OAK_PLANKS);
    assert(sel.count == 32);

    printf("  PASS test_get_selected_item\n");
}

static void test_get_selected_after_change(void)
{
    entity_id_t id = fresh_entity();

    /* Put different items in hotbar slots 0 and 3 */
    inventory_t *inv = mc_inventory_get(id);
    inv->slots[0].id    = BLOCK_STONE;
    inv->slots[0].count = 10;
    inv->slots[3].id    = BLOCK_DIRT;
    inv->slots[3].count = 5;

    mc_inventory_set_selected(id, 0);
    item_stack_t sel = mc_inventory_get_selected(id);
    assert(sel.id == BLOCK_STONE);
    assert(sel.count == 10);

    mc_inventory_set_selected(id, 3);
    sel = mc_inventory_get_selected(id);
    assert(sel.id == BLOCK_DIRT);
    assert(sel.count == 5);

    /* Empty slot */
    mc_inventory_set_selected(id, 7);
    sel = mc_inventory_get_selected(id);
    assert(sel.count == 0);

    printf("  PASS test_get_selected_after_change\n");
}

static void test_get_selected_invalid_entity(void)
{
    item_stack_t sel = mc_inventory_get_selected(ENTITY_INVALID);
    assert(sel.count == 0);
    assert(sel.id == 0);

    printf("  PASS test_get_selected_invalid_entity\n");
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    printf("mc_inventory tests:\n");

    test_get_valid();
    test_get_invalid();
    test_add_single_item();
    test_add_stacks_same_type();
    test_add_overflow_stack();
    test_add_different_types_no_stack();
    test_add_max_stack_size();
    test_add_inventory_full();
    test_add_zero_count();
    test_add_invalid_entity();
    test_remove_partial();
    test_remove_all();
    test_remove_more_than_present();
    test_remove_from_empty_slot();
    test_remove_invalid_slot();
    test_remove_invalid_entity();
    test_hotbar_selection_default();
    test_set_selected_valid();
    test_set_selected_out_of_range();
    test_set_selected_invalid_entity();
    test_get_selected_item();
    test_get_selected_after_change();
    test_get_selected_invalid_entity();

    printf("ALL PASSED\n");
    return 0;
}
