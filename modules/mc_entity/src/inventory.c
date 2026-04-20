/*
 * inventory.c — Inventory data model for the mc_entity ECS module.
 *
 * Each entity has an inventory with 36 slots (9 hotbar + 27 main) and a
 * selected-hotbar index.  Items stack up to MAX_STACK_SIZE (64).
 */

#include "mc_inventory.h"
#include "mc_entity.h"

/* --------------------------------------------------------------------- */
/* Static storage — one inventory per entity slot                         */
/* --------------------------------------------------------------------- */

static inventory_t inventories[MAX_ENTITIES];

/* --------------------------------------------------------------------- */
/* Helpers                                                                */
/* --------------------------------------------------------------------- */

static int valid_entity(entity_id_t id)
{
    return id > 0 && id < MAX_ENTITIES;
}

/* --------------------------------------------------------------------- */
/* Public API                                                             */
/* --------------------------------------------------------------------- */

inventory_t *mc_inventory_get(entity_id_t id)
{
    if (!valid_entity(id)) {
        return NULL;
    }
    return &inventories[id];
}

mc_error_t mc_inventory_add_item(entity_id_t id, block_id_t item,
                                 uint8_t count)
{
    if (!valid_entity(id) || count == 0) {
        return MC_ERR_INVALID_ARG;
    }

    inventory_t *inv = &inventories[id];
    uint8_t remaining = count;

    /* Pass 1: stack onto existing matching slots */
    for (int i = 0; i < INVENTORY_SIZE && remaining > 0; ++i) {
        if (inv->slots[i].id == item && inv->slots[i].count > 0 &&
            inv->slots[i].count < MAX_STACK_SIZE) {
            uint8_t space = (uint8_t)(MAX_STACK_SIZE - inv->slots[i].count);
            uint8_t add   = remaining < space ? remaining : space;
            inv->slots[i].count = (uint8_t)(inv->slots[i].count + add);
            remaining = (uint8_t)(remaining - add);
        }
    }

    /* Pass 2: fill first empty slot(s) */
    for (int i = 0; i < INVENTORY_SIZE && remaining > 0; ++i) {
        if (inv->slots[i].count == 0) {
            uint8_t add = remaining < MAX_STACK_SIZE
                              ? remaining
                              : (uint8_t)MAX_STACK_SIZE;
            inv->slots[i].id    = item;
            inv->slots[i].count = add;
            remaining = (uint8_t)(remaining - add);
        }
    }

    return remaining == 0 ? MC_OK : MC_ERR_FULL;
}

mc_error_t mc_inventory_remove_item(entity_id_t id, uint8_t slot,
                                    uint8_t count)
{
    if (!valid_entity(id) || slot >= INVENTORY_SIZE) {
        return MC_ERR_INVALID_ARG;
    }

    inventory_t *inv = &inventories[id];

    if (inv->slots[slot].count == 0) {
        return MC_ERR_EMPTY;
    }

    if (count >= inv->slots[slot].count) {
        inv->slots[slot].id    = 0;
        inv->slots[slot].count = 0;
    } else {
        inv->slots[slot].count = (uint8_t)(inv->slots[slot].count - count);
    }

    return MC_OK;
}

item_stack_t mc_inventory_get_selected(entity_id_t id)
{
    item_stack_t empty = {0, 0};

    if (!valid_entity(id)) {
        return empty;
    }

    const inventory_t *inv = &inventories[id];
    return inv->slots[inv->selected_hotbar];
}

mc_error_t mc_inventory_set_selected(entity_id_t id, uint8_t slot)
{
    if (!valid_entity(id) || slot >= HOTBAR_SLOTS) {
        return MC_ERR_INVALID_ARG;
    }

    inventories[id].selected_hotbar = slot;
    return MC_OK;
}
