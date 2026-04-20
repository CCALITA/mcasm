/*
 * pathfind_c.c — C fallback for A* pathfinding (used on non-x86_64 platforms).
 * On x86_64, pathfind.asm provides this function instead.
 *
 * Implements: uint8_t mc_mob_ai_pathfind(vec3_t start, vec3_t end,
 *                                         vec3_t* out_path, uint32_t max_steps,
 *                                         uint32_t* out_length)
 */

#include "mob_ai_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ── Constants ────────────────────────────────────────────────────── */

#define MAX_OPEN   4096
#define MAX_CLOSED 4096

/* ── Node for A* ──────────────────────────────────────────────────── */

typedef struct {
    int32_t x, y, z;
    int32_t parent_index;   /* index into closed[] or -1 */
    float   g, h, f;
} pf_node_t;

/* ── Static buffers (mirrors the ASM version) ─────────────────────── */

static pf_node_t open_set[MAX_OPEN];
static pf_node_t closed_set[MAX_CLOSED];
static uint32_t  open_count;
static uint32_t  closed_count;

/* ── 6-connected neighbors ────────────────────────────────────────── */

static const int32_t dx[6] = { 1, -1,  0,  0,  0,  0};
static const int32_t dy[6] = { 0,  0,  1, -1,  0,  0};
static const int32_t dz[6] = { 0,  0,  0,  0,  1, -1};

/* ── Helpers ──────────────────────────────────────────────────────── */

static float manhattan(int32_t x1, int32_t y1, int32_t z1,
                       int32_t x2, int32_t y2, int32_t z2)
{
    return (float)(abs(x1 - x2) + abs(y1 - y2) + abs(z1 - z2));
}

static int is_walkable(int32_t x, int32_t y, int32_t z)
{
    if (!g_block_query) return 0;

    block_pos_t pos = {x, y, z, 0};
    block_id_t  blk = g_block_query(pos);

    if (blk != BLOCK_AIR) return 0;

    /* Floor below must be solid */
    if (y <= 0) return 0;
    block_pos_t below = {x, y - 1, z, 0};
    block_id_t  floor_blk = g_block_query(below);
    if (floor_blk == BLOCK_AIR || floor_blk == BLOCK_WATER || floor_blk == BLOCK_LAVA) {
        return 0;
    }
    return 1;
}

static int32_t find_lowest_f(void)
{
    if (open_count == 0) return -1;
    int32_t best = 0;
    float   best_f = open_set[0].f;
    for (uint32_t i = 1; i < open_count; i++) {
        if (open_set[i].f < best_f) {
            best_f = open_set[i].f;
            best = (int32_t)i;
        }
    }
    return best;
}

static int32_t find_in_set(const pf_node_t *set, uint32_t count,
                           int32_t x, int32_t y, int32_t z)
{
    for (uint32_t i = 0; i < count; i++) {
        if (set[i].x == x && set[i].y == y && set[i].z == z) {
            return (int32_t)i;
        }
    }
    return -1;
}

/* ── Public API ───────────────────────────────────────────────────── */

uint8_t mc_mob_ai_pathfind(vec3_t start, vec3_t end,
                           vec3_t *out_path, uint32_t max_steps,
                           uint32_t *out_length)
{
    int32_t sx = (int32_t)start.x;
    int32_t sy = (int32_t)start.y;
    int32_t sz = (int32_t)start.z;
    int32_t ex = (int32_t)end.x;
    int32_t ey = (int32_t)end.y;
    int32_t ez = (int32_t)end.z;

    open_count   = 0;
    closed_count = 0;

    /* Seed open set with start node */
    float h0 = manhattan(sx, sy, sz, ex, ey, ez);
    open_set[0] = (pf_node_t){sx, sy, sz, -1, 0.0f, h0, h0};
    open_count  = 1;

    for (uint32_t iter = 0; iter < max_steps; iter++) {
        int32_t bi = find_lowest_f();
        if (bi < 0) break;  /* open set empty */

        pf_node_t cur = open_set[bi];

        /* Goal check */
        if (cur.x == ex && cur.y == ey && cur.z == ez) {
            /* Move goal to closed for tracing */
            if (closed_count < MAX_CLOSED) {
                closed_set[closed_count] = cur;
                uint32_t goal_ci = closed_count;
                closed_count++;

                /* Trace path */
                int32_t trace[256];
                int32_t tlen = 0;
                int32_t ci = (int32_t)goal_ci;
                while (ci >= 0 && tlen < 256) {
                    trace[tlen++] = ci;
                    ci = closed_set[ci].parent_index;
                }

                /* Copy in forward order */
                uint32_t plen = (uint32_t)tlen;
                for (uint32_t i = 0; i < plen; i++) {
                    int32_t ni = trace[tlen - 1 - (int32_t)i];
                    out_path[i] = (vec3_t){
                        (float)closed_set[ni].x,
                        (float)closed_set[ni].y,
                        (float)closed_set[ni].z,
                        0.0f
                    };
                }
                *out_length = plen;
                return 1;
            }
            break;
        }

        /* Move to closed */
        if (closed_count >= MAX_CLOSED) break;
        closed_set[closed_count] = cur;
        uint32_t cur_ci = closed_count;
        closed_count++;

        /* Remove from open (swap with last) */
        open_count--;
        if ((uint32_t)bi < open_count) {
            open_set[bi] = open_set[open_count];
        }

        /* Explore neighbors */
        for (int d = 0; d < 6; d++) {
            int32_t nx = cur.x + dx[d];
            int32_t ny = cur.y + dy[d];
            int32_t nz = cur.z + dz[d];

            if (find_in_set(closed_set, closed_count, nx, ny, nz) >= 0)
                continue;

            if (!is_walkable(nx, ny, nz))
                continue;

            float tent_g = cur.g + 1.0f;
            int32_t oi = find_in_set(open_set, open_count, nx, ny, nz);

            if (oi >= 0) {
                if (tent_g < open_set[oi].g) {
                    open_set[oi].g = tent_g;
                    open_set[oi].f = tent_g + open_set[oi].h;
                    open_set[oi].parent_index = (int32_t)cur_ci;
                }
            } else if (open_count < MAX_OPEN) {
                float nh = manhattan(nx, ny, nz, ex, ey, ez);
                open_set[open_count] = (pf_node_t){
                    nx, ny, nz, (int32_t)cur_ci, tent_g, nh, tent_g + nh
                };
                open_count++;
            }
        }
    }

    *out_length = 0;
    return 0;
}
