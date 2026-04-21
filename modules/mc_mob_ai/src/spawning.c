#include "mob_ai_internal.h"
#include "mc_entity.h"
#include <math.h>

/* ── Constants ───────────────────────────────────────────────────── */

#define SPAWN_INTERVAL       20    /* ticks between spawn attempts (~1 s) */
#define SPAWN_RANGE_MIN      24    /* minimum distance from player        */
#define SPAWN_RANGE_MAX      128   /* maximum distance from player        */
#define MAX_SPAWNED_MOBS     50    /* cap on total living mobs            */
#define LIGHT_THRESHOLD      8     /* hostile < 8, passive >= 8           */
#define SPAWN_ATTEMPTS       4     /* random positions tried per tick     */
#define SKY_PROBE_HEIGHT     4     /* blocks above to check for sky       */

/* ── PRNG (xorshift32) ──────────────────────────────────────────── */

static uint32_t s_rng_state = 1;

static void spawning_seed(uint32_t seed)
{
    s_rng_state = seed ? seed : 1;
}

static uint32_t rng_next(void)
{
    uint32_t x = s_rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_rng_state = x;
    return x;
}

static float rng_range(float lo, float hi)
{
    float t = (float)(rng_next() & 0xFFFFu) / 65536.0f;
    return lo + t * (hi - lo);
}

/* ── Mob type tables ─────────────────────────────────────────────── */

static const uint8_t s_hostile_types[] = {
    MOB_ZOMBIE, MOB_SKELETON, MOB_CREEPER, MOB_SPIDER
};

static const uint8_t s_passive_types[] = {
    MOB_PIG, MOB_COW, MOB_SHEEP, MOB_CHICKEN
};

#define HOSTILE_COUNT ((uint32_t)(sizeof(s_hostile_types) / sizeof(s_hostile_types[0])))
#define PASSIVE_COUNT ((uint32_t)(sizeof(s_passive_types) / sizeof(s_passive_types[0])))

/* ── Light query helper ──────────────────────────────────────────── */

/* Approximates light by probing for sky exposure above `pos`.
   Returns 0 (dark / covered) or 15 (open sky). */
static uint8_t query_light_level(block_pos_t pos)
{
    if (!g_block_query) {
        return 15; /* fail-open: prevents hostile spam without a world */
    }

    for (int32_t dy = 1; dy <= SKY_PROBE_HEIGHT; dy++) {
        block_pos_t above = { pos.x, pos.y + dy, pos.z, 0 };
        block_id_t  bid   = g_block_query(above);
        if (mc_block_is_solid(bid)) {
            return 0;
        }
    }
    return 15;
}

/* ── Count active mobs ───────────────────────────────────────────── */

static uint32_t count_active_mobs(void)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < g_mob_count; i++) {
        if (g_mobs[i].active) {
            n++;
        }
    }
    return n;
}

/* ── Try spawning a single mob at a candidate position ───────────── */

static uint8_t try_spawn_at(vec3_t pos)
{
    block_pos_t feet = {
        (int32_t)floorf(pos.x),
        (int32_t)floorf(pos.y),
        (int32_t)floorf(pos.z),
        0
    };

    if (g_block_query) {
        block_id_t at_feet = g_block_query(feet);
        if (at_feet != BLOCK_AIR) {
            return 0;
        }
    }

    block_pos_t below  = { feet.x, feet.y - 1, feet.z, 0 };
    block_id_t  ground = g_block_query ? g_block_query(below) : BLOCK_STONE;
    if (!mc_block_is_solid(ground)) {
        return 0;
    }

    uint8_t light = query_light_level(feet);

    uint8_t mob_type;
    if (light < LIGHT_THRESHOLD) {
        mob_type = s_hostile_types[rng_next() % HOSTILE_COUNT];
    } else if (ground == BLOCK_GRASS) {
        mob_type = s_passive_types[rng_next() % PASSIVE_COUNT];
    } else {
        return 0;
    }

    uint32_t mask = COMPONENT_TRANSFORM | COMPONENT_PHYSICS |
                    COMPONENT_HEALTH    | COMPONENT_AI;
    entity_id_t eid = mc_entity_create(mask);
    if (eid == ENTITY_INVALID) {
        return 0;
    }

    mc_entity_set_position(eid, pos);

    if (mc_mob_ai_assign(eid, mob_type) != MC_OK) {
        mc_entity_destroy(eid);
        return 0;
    }

    mob_entry_t *mob = mob_find(eid);
    if (mob) {
        mob->position = pos;
    }

    return 1;
}

/* ── Public API ──────────────────────────────────────────────────── */

void mc_mob_spawning_init(void)
{
    spawning_seed(12345u);
}

void mc_mob_spawning_tick(tick_t tick, vec3_t player_pos)
{
    if (tick % SPAWN_INTERVAL != 0) {
        return;
    }

    uint32_t active = count_active_mobs();

    for (uint32_t attempt = 0;
         attempt < SPAWN_ATTEMPTS && active < MAX_SPAWNED_MOBS;
         attempt++) {

        float angle = rng_range(0.0f, 6.2831853f);
        float dist  = rng_range((float)SPAWN_RANGE_MIN,
                                (float)SPAWN_RANGE_MAX);

        vec3_t candidate = {
            player_pos.x + cosf(angle) * dist,
            player_pos.y,
            player_pos.z + sinf(angle) * dist,
            0.0f
        };

        active += try_spawn_at(candidate);
    }
}
