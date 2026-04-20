#include "mob_ai_internal.h"
#include <stdlib.h>

/* ── Behavior tree tick ───────────────────────────────────────────── */

bt_status_t bt_tick(bt_node_t *node, entity_id_t entity, tick_t tick)
{
    if (!node) {
        return BT_FAILURE;
    }

    switch (node->type) {

    case BT_ACTION:
        if (!node->action) {
            return BT_FAILURE;
        }
        return node->action(entity, tick);

    case BT_SEQUENCE: {
        uint8_t start = node->running_child;
        for (uint8_t i = start; i < node->child_count; i++) {
            bt_status_t s = bt_tick(node->children[i], entity, tick);
            if (s == BT_RUNNING) {
                node->running_child = i;
                return BT_RUNNING;
            }
            if (s == BT_FAILURE) {
                node->running_child = 0;
                return BT_FAILURE;
            }
            /* BT_SUCCESS → continue to next child */
        }
        node->running_child = 0;
        return BT_SUCCESS;
    }

    case BT_SELECTOR: {
        uint8_t start = node->running_child;
        for (uint8_t i = start; i < node->child_count; i++) {
            bt_status_t s = bt_tick(node->children[i], entity, tick);
            if (s == BT_RUNNING) {
                node->running_child = i;
                return BT_RUNNING;
            }
            if (s == BT_SUCCESS) {
                node->running_child = 0;
                return BT_SUCCESS;
            }
            /* BT_FAILURE → try next child */
        }
        node->running_child = 0;
        return BT_FAILURE;
    }

    }
    return BT_FAILURE;
}

/* ── Node constructors ────────────────────────────────────────────── */

static bt_node_t *bt_node_alloc(bt_node_type_t type)
{
    bt_node_t *n = calloc(1, sizeof(bt_node_t));
    if (n) {
        n->type = type;
    }
    return n;
}

bt_node_t *bt_action_new(bt_action_fn action)
{
    bt_node_t *n = bt_node_alloc(BT_ACTION);
    if (n) {
        n->action = action;
    }
    return n;
}

bt_node_t *bt_sequence_new(bt_node_t **children, uint8_t count)
{
    bt_node_t *n = bt_node_alloc(BT_SEQUENCE);
    if (!n) {
        return NULL;
    }
    uint8_t c = (count > BT_MAX_CHILDREN) ? BT_MAX_CHILDREN : count;
    for (uint8_t i = 0; i < c; i++) {
        n->children[i] = children[i];
    }
    n->child_count = c;
    return n;
}

bt_node_t *bt_selector_new(bt_node_t **children, uint8_t count)
{
    bt_node_t *n = bt_node_alloc(BT_SELECTOR);
    if (!n) {
        return NULL;
    }
    uint8_t c = (count > BT_MAX_CHILDREN) ? BT_MAX_CHILDREN : count;
    for (uint8_t i = 0; i < c; i++) {
        n->children[i] = children[i];
    }
    n->child_count = c;
    return n;
}
