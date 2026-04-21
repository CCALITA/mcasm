#ifndef MOB_AI_INTERNAL_H
#define MOB_AI_INTERNAL_H

#include "mc_mob_ai.h"
#include "mc_block.h"

/* ── Pathfinding (implemented in pathfind.asm) ────────────────────────── */

/* The ASM pathfinder calls this C function to query walkability. */
extern mc_ai_block_query_fn g_block_query;

/* ── Behavior tree ────────────────────────────────────────────────────── */

typedef enum {
    BT_SUCCESS  = 0,
    BT_FAILURE  = 1,
    BT_RUNNING  = 2
} bt_status_t;

typedef enum {
    BT_ACTION,
    BT_SEQUENCE,
    BT_SELECTOR
} bt_node_type_t;

#define BT_MAX_CHILDREN 8

struct bt_node;

typedef bt_status_t (*bt_action_fn)(entity_id_t entity, tick_t tick);

typedef struct bt_node {
    bt_node_type_t   type;
    bt_action_fn     action;          /* only for BT_ACTION nodes */
    struct bt_node  *children[BT_MAX_CHILDREN];
    uint8_t          child_count;
    uint8_t          running_child;   /* tracks which child was RUNNING */
} bt_node_t;

bt_status_t bt_tick(bt_node_t *node, entity_id_t entity, tick_t tick);

bt_node_t *bt_action_new(bt_action_fn action);
bt_node_t *bt_sequence_new(bt_node_t **children, uint8_t count);
bt_node_t *bt_selector_new(bt_node_t **children, uint8_t count);

/* ── Mob registry ─────────────────────────────────────────────────────── */

#define MAX_MOB_ENTITIES 512

typedef struct {
    entity_id_t entity_id;
    uint8_t     mob_type;
    uint8_t     active;
    uint8_t     state;          /* mob-specific FSM state */
    uint8_t     _pad;
    vec3_t      position;
    vec3_t      target_pos;
    tick_t      last_action_tick;
    tick_t      cooldown_until;
} mob_entry_t;

extern mob_entry_t g_mobs[MAX_MOB_ENTITIES];
extern uint32_t    g_mob_count;

/* Look up a mob entry by entity id (defined in mobs.c). */
mob_entry_t *mob_find(entity_id_t entity);

/* Mob-specific behavior trees (built once in mobs.c) */
bt_node_t *mob_build_zombie_tree(void);
bt_node_t *mob_build_skeleton_tree(void);
bt_node_t *mob_build_creeper_tree(void);
bt_node_t *mob_build_passive_tree(void);

#endif /* MOB_AI_INTERNAL_H */
