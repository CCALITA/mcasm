#ifndef MC_INVENTORY_H
#define MC_INVENTORY_H

#include "mc_types.h"
#include "mc_error.h"

#define INVENTORY_SIZE       36   /* 9 hotbar + 27 main */
#define MAX_STACK_SIZE       64
#define HOTBAR_SLOTS          9

typedef struct {
    block_id_t id;
    uint8_t    count;
} item_stack_t;

typedef struct {
    item_stack_t slots[INVENTORY_SIZE];
    uint8_t      selected_hotbar;   /* 0-8 */
} inventory_t;

/*
 * mc_inventory_get — return a pointer to the inventory for entity |id|.
 * Returns NULL if |id| is out of range.
 */
inventory_t* mc_inventory_get(entity_id_t id);

/*
 * mc_inventory_add_item — add |count| items of |item| to the inventory of
 * entity |id|.  Stacks onto an existing matching slot first (up to
 * MAX_STACK_SIZE), then fills the first empty slot.
 *
 * Returns MC_OK on success, MC_ERR_FULL if no room, MC_ERR_INVALID_ARG for
 * bad |id| or zero |count|.
 */
mc_error_t mc_inventory_add_item(entity_id_t id, block_id_t item,
                                 uint8_t count);

/*
 * mc_inventory_remove_item — remove |count| items from |slot| in the
 * inventory of entity |id|.  If |count| >= slot count the slot is cleared.
 *
 * Returns MC_OK on success, MC_ERR_INVALID_ARG for bad |id|/|slot|,
 * MC_ERR_EMPTY if the slot is already empty.
 */
mc_error_t mc_inventory_remove_item(entity_id_t id, uint8_t slot,
                                    uint8_t count);

/*
 * mc_inventory_get_selected — return the item stack in the currently
 * selected hotbar slot.  Returns a zero-initialised stack if |id| is invalid.
 */
item_stack_t mc_inventory_get_selected(entity_id_t id);

/*
 * mc_inventory_set_selected — set the selected hotbar slot (0-8).
 * Returns MC_ERR_INVALID_ARG for bad |id| or |slot| >= HOTBAR_SLOTS.
 */
mc_error_t mc_inventory_set_selected(entity_id_t id, uint8_t slot);

#endif /* MC_INVENTORY_H */
