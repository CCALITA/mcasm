#include "mc_main_internal.h"

#include "mc_world.h"
#include "mc_render.h"

#include <string.h>

/* ---- Constants ---- */

#define MAX_TRACKED_CHUNKS  1024
#define MAP_CAPACITY        2048  /* must be power of two, > MAX_TRACKED_CHUNKS */
#define MAX_MESH_HANDLES    (MAX_TRACKED_CHUNKS * SECTION_COUNT)
#define MAX_DIRTY_PER_FRAME 64

/* ---- Per-chunk entry: mesh handles for each section ---- */

typedef struct {
    chunk_pos_t pos;
    uint64_t    meshes[SECTION_COUNT];   /* 0 = no mesh */
    uint8_t     occupied;                /* 1 = slot in use */
} chunk_entry_t;

/* ---- Open-addressing hash map ---- */

static chunk_entry_t g_map[MAP_CAPACITY];
static uint32_t      g_entry_count;

/* Scratch buffer for mc_render_draw_terrain */
static uint64_t g_handle_buf[MAX_MESH_HANDLES];
static uint32_t g_handle_count;

/* ---- Hash helpers ---- */

static uint32_t hash_pos(chunk_pos_t pos)
{
    uint32_t h = (uint32_t)pos.x * 73856093u ^ (uint32_t)pos.z * 19349663u;
    return h & (MAP_CAPACITY - 1);
}

static int pos_eq(chunk_pos_t a, chunk_pos_t b)
{
    return a.x == b.x && a.z == b.z;
}

static chunk_entry_t *map_find(chunk_pos_t pos)
{
    uint32_t idx = hash_pos(pos);
    for (uint32_t i = 0; i < MAP_CAPACITY; i++) {
        uint32_t slot = (idx + i) & (MAP_CAPACITY - 1);
        chunk_entry_t *e = &g_map[slot];
        if (!e->occupied) {
            return NULL;
        }
        if (pos_eq(e->pos, pos)) {
            return e;
        }
    }
    return NULL;
}

static chunk_entry_t *map_insert(chunk_pos_t pos)
{
    uint32_t idx = hash_pos(pos);
    for (uint32_t i = 0; i < MAP_CAPACITY; i++) {
        uint32_t slot = (idx + i) & (MAP_CAPACITY - 1);
        chunk_entry_t *e = &g_map[slot];
        if (!e->occupied) {
            memset(e, 0, sizeof(*e));
            e->pos = pos;
            e->occupied = 1;
            g_entry_count++;
            return e;
        }
        if (pos_eq(e->pos, pos)) {
            return e;
        }
    }
    return NULL;  /* table full */
}

static void map_remove_at(uint32_t slot)
{
    g_map[slot].occupied = 0;
    g_entry_count--;

    /* Rehash subsequent entries in the probing chain */
    uint32_t next = (slot + 1) & (MAP_CAPACITY - 1);
    while (g_map[next].occupied) {
        chunk_entry_t displaced = g_map[next];
        g_map[next].occupied = 0;
        g_entry_count--;

        /* Re-insert */
        uint32_t idx = hash_pos(displaced.pos);
        for (uint32_t i = 0; i < MAP_CAPACITY; i++) {
            uint32_t s = (idx + i) & (MAP_CAPACITY - 1);
            if (!g_map[s].occupied) {
                g_map[s] = displaced;
                g_entry_count++;
                break;
            }
        }
        next = (next + 1) & (MAP_CAPACITY - 1);
    }
}

static void free_entry_meshes(chunk_entry_t *e)
{
    for (uint32_t s = 0; s < SECTION_COUNT; s++) {
        if (e->meshes[s] != 0) {
            mc_render_free_mesh(e->meshes[s]);
            e->meshes[s] = 0;
        }
    }
}

/* ---- Public API ---- */

void chunk_mesh_mgr_init(void)
{
    memset(g_map, 0, sizeof(g_map));
    g_entry_count  = 0;
    g_handle_count = 0;
}

void chunk_mesh_mgr_update(void)
{
    chunk_pos_t dirty_positions[MAX_DIRTY_PER_FRAME];
    uint32_t    dirty_count = 0;

    mc_world_get_dirty_chunks(dirty_positions, MAX_DIRTY_PER_FRAME, &dirty_count);

    for (uint32_t i = 0; i < dirty_count; i++) {
        chunk_pos_t pos = dirty_positions[i];
        const chunk_t *chunk = mc_world_get_chunk(pos);
        if (!chunk || chunk->state < CHUNK_STATE_READY) {
            continue;
        }

        uint32_t mask = chunk->dirty_mask;
        if (mask == 0) {
            continue;
        }

        chunk_entry_t *entry = map_find(pos);
        if (!entry) {
            entry = map_insert(pos);
            if (!entry) {
                continue;  /* map full */
            }
        }

        for (uint8_t sy = 0; sy < SECTION_COUNT; sy++) {
            if (!(mask & (1u << sy))) {
                continue;
            }

            if (entry->meshes[sy] != 0) {
                mc_render_free_mesh(entry->meshes[sy]);
                entry->meshes[sy] = 0;
            }

            const chunk_section_t *section = &chunk->sections[sy];
            if (section->non_air_count == 0) {
                continue;  /* empty section, no mesh needed */
            }

            uint64_t handle = mc_render_build_chunk_mesh(section, pos, sy);
            entry->meshes[sy] = handle;
        }
    }

    /* Detect unloaded chunks and free their meshes */
    chunk_pos_t unloaded[MAX_TRACKED_CHUNKS];
    uint32_t    unloaded_count = 0;

    for (uint32_t slot = 0; slot < MAP_CAPACITY; slot++) {
        const chunk_entry_t *e = &g_map[slot];
        if (!e->occupied) {
            continue;
        }
        const chunk_t *chunk = mc_world_get_chunk(e->pos);
        if (!chunk) {
            unloaded[unloaded_count++] = e->pos;
        }
    }

    for (uint32_t i = 0; i < unloaded_count; i++) {
        chunk_entry_t *e = map_find(unloaded[i]);
        if (e) {
            free_entry_meshes(e);
            uint32_t idx = (uint32_t)(e - g_map);
            map_remove_at(idx);
        }
    }

    /* Rebuild the flat handle array */
    g_handle_count = 0;
    for (uint32_t slot = 0; slot < MAP_CAPACITY; slot++) {
        const chunk_entry_t *e = &g_map[slot];
        if (!e->occupied) {
            continue;
        }

        for (uint32_t s = 0; s < SECTION_COUNT; s++) {
            if (e->meshes[s] != 0) {
                g_handle_buf[g_handle_count++] = e->meshes[s];
            }
        }
    }
}

void chunk_mesh_mgr_get_meshes(uint64_t **out_handles, uint32_t *out_count)
{
    if (out_handles) {
        *out_handles = (g_handle_count > 0) ? g_handle_buf : NULL;
    }
    if (out_count) {
        *out_count = g_handle_count;
    }
}

void chunk_mesh_mgr_shutdown(void)
{
    for (uint32_t slot = 0; slot < MAP_CAPACITY; slot++) {
        chunk_entry_t *e = &g_map[slot];
        if (e->occupied) {
            free_entry_meshes(e);
            e->occupied = 0;
        }
    }
    g_entry_count  = 0;
    g_handle_count = 0;
}
