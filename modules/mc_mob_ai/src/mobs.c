#include "mob_ai_internal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Global state ─────────────────────────────────────────────────── */

mc_ai_block_query_fn g_block_query = NULL;
mob_entry_t          g_mobs[MAX_MOB_ENTITIES];
uint32_t             g_mob_count = 0;

static bt_node_t *g_trees[MOB_TYPE_COUNT];

/* ── Helpers ──────────────────────────────────────────────────────── */

static float vec3_dist_sq(vec3_t a, vec3_t b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

static vec3_t vec3_towards(vec3_t from, vec3_t to, float speed)
{
    float d2 = vec3_dist_sq(from, to);
    float d  = sqrtf(d2);
    if (d < 0.001f) {
        return from;
    }
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float dz = to.z - from.z;
    float s = (speed < d) ? speed : d;
    vec3_t result = {
        from.x + (dx / d) * s,
        from.y + (dy / d) * s,
        from.z + (dz / d) * s,
        0.0f
    };
    return result;
}

static mob_entry_t *find_mob(entity_id_t entity)
{
    for (uint32_t i = 0; i < g_mob_count; i++) {
        if (g_mobs[i].active && g_mobs[i].entity_id == entity) {
            return &g_mobs[i];
        }
    }
    return NULL;
}

/* Simulated player position — in a real system this would come from
   the entity module.  For now, mobs target (0, 4, 0). */
static vec3_t get_player_pos(void)
{
    vec3_t p = {0.0f, 4.0f, 0.0f, 0.0f};
    return p;
}

/* ── Mob states ───────────────────────────────────────────────────── */

#define STATE_IDLE      0
#define STATE_CHASE     1
#define STATE_ATTACK    2
#define STATE_FLEE      3
#define STATE_WANDER    4
#define STATE_EXPLODE   5

/* ── Action helpers (shared by behaviour trees) ───────────────────── */

static bt_status_t action_idle(entity_id_t entity, tick_t tick)
{
    (void)entity;
    (void)tick;
    return BT_SUCCESS;
}

#define DETECT_RANGE_SQ  (16.0f * 16.0f)

static bt_status_t detect_player(entity_id_t entity, tick_t tick)
{
    (void)tick;
    mob_entry_t *mob = find_mob(entity);
    if (!mob) return BT_FAILURE;

    vec3_t player = get_player_pos();
    float d2 = vec3_dist_sq(mob->position, player);
    if (d2 <= DETECT_RANGE_SQ) {
        mob->target_pos = player;
        return BT_SUCCESS;
    }
    return BT_FAILURE;
}

/* ── Zombie actions ───────────────────────────────────────────────── */static bt_status_t zombie_chase(entity_id_t entity, tick_t tick)
{
    (void)tick;
    mob_entry_t *mob = find_mob(entity);
    if (!mob) return BT_FAILURE;

    float d2 = vec3_dist_sq(mob->position, mob->target_pos);
    if (d2 <= 1.5f * 1.5f) {
        mob->state = STATE_ATTACK;
        return BT_SUCCESS;
    }
    mob->position = vec3_towards(mob->position, mob->target_pos, 0.4f);
    mob->state = STATE_CHASE;
    return BT_RUNNING;
}

static bt_status_t zombie_attack(entity_id_t entity, tick_t tick)
{
    mob_entry_t *mob = find_mob(entity);
    if (!mob) return BT_FAILURE;

    if (tick < mob->cooldown_until) {
        return BT_RUNNING;
    }
    mob->state = STATE_ATTACK;
    mob->cooldown_until = tick + 20; /* 1-second cooldown at 20 tps */
    return BT_SUCCESS;
}

/* ── Skeleton actions ─────────────────────────────────────────────── */

static bt_status_t skeleton_maintain_distance(entity_id_t entity, tick_t tick)
{
    (void)tick;
    mob_entry_t *mob = find_mob(entity);
    if (!mob) return BT_FAILURE;

    float d2 = vec3_dist_sq(mob->position, mob->target_pos);
    float d  = sqrtf(d2);

    if (d < 8.0f) {
        /* Too close — back away */
        vec3_t away = {
            mob->position.x + (mob->position.x - mob->target_pos.x) * 0.3f,
            mob->position.y,
            mob->position.z + (mob->position.z - mob->target_pos.z) * 0.3f,
            0.0f
        };
        mob->position = vec3_towards(mob->position, away, 0.3f);
        mob->state = STATE_FLEE;
        return BT_RUNNING;
    }
    if (d > 16.0f) {
        /* Too far — approach */
        mob->position = vec3_towards(mob->position, mob->target_pos, 0.3f);
        mob->state = STATE_CHASE;
        return BT_RUNNING;
    }
    /* In sweet spot (8-16 blocks) */
    mob->state = STATE_IDLE;
    return BT_SUCCESS;
}

static bt_status_t skeleton_shoot(entity_id_t entity, tick_t tick)
{
    mob_entry_t *mob = find_mob(entity);
    if (!mob) return BT_FAILURE;

    if (tick < mob->cooldown_until) {
        return BT_RUNNING;
    }
    mob->state = STATE_ATTACK;
    mob->cooldown_until = tick + 30; /* 1.5-second bow cooldown */
    return BT_SUCCESS;
}

/* ── Creeper actions ──────────────────────────────────────────────── */

static bt_status_t creeper_approach(entity_id_t entity, tick_t tick)
{
    (void)tick;
    mob_entry_t *mob = find_mob(entity);
    if (!mob) return BT_FAILURE;

    float d2 = vec3_dist_sq(mob->position, mob->target_pos);
    if (d2 <= 3.0f * 3.0f) {
        return BT_SUCCESS;
    }
    mob->position = vec3_towards(mob->position, mob->target_pos, 0.35f);
    mob->state = STATE_CHASE;
    return BT_RUNNING;
}

static bt_status_t creeper_explode(entity_id_t entity, tick_t tick)
{
    mob_entry_t *mob = find_mob(entity);
    if (!mob) return BT_FAILURE;

    /* Fuse: 30 ticks (1.5 seconds) */
    if (mob->state != STATE_EXPLODE) {
        mob->state = STATE_EXPLODE;
        mob->cooldown_until = tick + 30;
        return BT_RUNNING;
    }
    if (tick < mob->cooldown_until) {
        return BT_RUNNING;
    }
    /* Boom — mark inactive */
    mob->active = 0;
    return BT_SUCCESS;
}

/* ── Passive mob actions ──────────────────────────────────────────── */

static bt_status_t passive_wander(entity_id_t entity, tick_t tick)
{
    mob_entry_t *mob = find_mob(entity);
    if (!mob) return BT_FAILURE;

    /* Pick a new wander target every 60 ticks (3 seconds) */
    if (tick >= mob->cooldown_until) {
        /* Simple deterministic "random" wander based on entity + tick */
        uint32_t seed = (uint32_t)entity * 2654435761u + tick;
        float dx = (float)((int32_t)(seed % 11) - 5);
        float dz = (float)((int32_t)((seed >> 16) % 11) - 5);
        mob->target_pos.x = mob->position.x + dx;
        mob->target_pos.y = mob->position.y;
        mob->target_pos.z = mob->position.z + dz;
        mob->cooldown_until = tick + 60;
    }

    float d2 = vec3_dist_sq(mob->position, mob->target_pos);
    if (d2 > 1.0f) {
        mob->position = vec3_towards(mob->position, mob->target_pos, 0.2f);
        mob->state = STATE_WANDER;
        return BT_RUNNING;
    }
    mob->state = STATE_IDLE;
    return BT_SUCCESS;
}

/* ── Build behavior trees ─────────────────────────────────────────── */

bt_node_t *mob_build_zombie_tree(void)
{
    bt_node_t *detect = bt_action_new(detect_player);
    bt_node_t *chase  = bt_action_new(zombie_chase);
    bt_node_t *attack = bt_action_new(zombie_attack);

    bt_node_t *seq_children[] = {detect, chase, attack};
    bt_node_t *chase_seq = bt_sequence_new(seq_children, 3);

    bt_node_t *idle = bt_action_new(action_idle);
    bt_node_t *sel_children[] = {chase_seq, idle};
    return bt_selector_new(sel_children, 2);
}

bt_node_t *mob_build_skeleton_tree(void)
{
    bt_node_t *detect   = bt_action_new(detect_player);
    bt_node_t *maintain = bt_action_new(skeleton_maintain_distance);
    bt_node_t *shoot    = bt_action_new(skeleton_shoot);

    bt_node_t *seq_children[] = {detect, maintain, shoot};
    bt_node_t *combat_seq = bt_sequence_new(seq_children, 3);

    bt_node_t *idle = bt_action_new(action_idle);
    bt_node_t *sel_children[] = {combat_seq, idle};
    return bt_selector_new(sel_children, 2);
}

bt_node_t *mob_build_creeper_tree(void)
{
    bt_node_t *detect   = bt_action_new(detect_player);
    bt_node_t *approach = bt_action_new(creeper_approach);
    bt_node_t *explode  = bt_action_new(creeper_explode);

    bt_node_t *seq_children[] = {detect, approach, explode};
    bt_node_t *explode_seq = bt_sequence_new(seq_children, 3);

    bt_node_t *idle = bt_action_new(action_idle);
    bt_node_t *sel_children[] = {explode_seq, idle};
    return bt_selector_new(sel_children, 2);
}

bt_node_t *mob_build_passive_tree(void)
{
    return bt_action_new(passive_wander);
}

/* ── Free a tree recursively ──────────────────────────────────────── */

static void bt_free(bt_node_t *node)
{
    if (!node) return;
    for (uint8_t i = 0; i < node->child_count; i++) {
        bt_free(node->children[i]);
    }
    free(node);
}

/* ── Module API ───────────────────────────────────────────────────── */

mc_error_t mc_mob_ai_init(mc_ai_block_query_fn block_query)
{
    if (!block_query) {
        return MC_ERR_INVALID_ARG;
    }
    g_block_query = block_query;
    memset(g_mobs, 0, sizeof(g_mobs));
    g_mob_count = 0;

    /* Build per-type behavior trees */
    g_trees[MOB_ZOMBIE]   = mob_build_zombie_tree();
    g_trees[MOB_SKELETON] = mob_build_skeleton_tree();
    g_trees[MOB_CREEPER]  = mob_build_creeper_tree();
    g_trees[MOB_SPIDER]   = mob_build_passive_tree();   /* simplified */
    g_trees[MOB_ENDERMAN] = mob_build_passive_tree();   /* simplified */
    g_trees[MOB_PIG]      = mob_build_passive_tree();
    g_trees[MOB_COW]      = mob_build_passive_tree();
    g_trees[MOB_SHEEP]    = mob_build_passive_tree();
    g_trees[MOB_CHICKEN]  = mob_build_passive_tree();
    g_trees[MOB_VILLAGER] = mob_build_passive_tree();   /* simplified */

    return MC_OK;
}

void mc_mob_ai_shutdown(void)
{
    for (int i = 0; i < MOB_TYPE_COUNT; i++) {
        bt_free(g_trees[i]);
        g_trees[i] = NULL;
    }
    memset(g_mobs, 0, sizeof(g_mobs));
    g_mob_count   = 0;
    g_block_query = NULL;
}

mc_error_t mc_mob_ai_assign(entity_id_t entity, uint8_t mob_type)
{
    if (mob_type >= MOB_TYPE_COUNT) {
        return MC_ERR_INVALID_ARG;
    }
    /* Check duplicate */
    if (find_mob(entity)) {
        return MC_ERR_ALREADY_EXISTS;
    }
    if (g_mob_count >= MAX_MOB_ENTITIES) {
        return MC_ERR_FULL;
    }

    /* Find first inactive slot or append */
    mob_entry_t *slot = NULL;
    for (uint32_t i = 0; i < g_mob_count; i++) {
        if (!g_mobs[i].active) {
            slot = &g_mobs[i];
            break;
        }
    }
    if (!slot) {
        slot = &g_mobs[g_mob_count];
        g_mob_count++;
    }

    memset(slot, 0, sizeof(*slot));
    slot->entity_id = entity;
    slot->mob_type  = mob_type;
    slot->active    = 1;
    slot->state     = STATE_IDLE;
    return MC_OK;
}

void mc_mob_ai_remove(entity_id_t entity)
{
    mob_entry_t *mob = find_mob(entity);
    if (mob) {
        mob->active = 0;
    }
}

void mc_mob_ai_tick(tick_t current_tick)
{
    for (uint32_t i = 0; i < g_mob_count; i++) {
        if (!g_mobs[i].active) continue;

        uint8_t type = g_mobs[i].mob_type;
        if (type < MOB_TYPE_COUNT && g_trees[type]) {
            bt_tick(g_trees[type], g_mobs[i].entity_id, current_tick);
        }
    }
}
