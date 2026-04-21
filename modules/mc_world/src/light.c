#include "mc_world.h"
#include "mc_block.h"
#include "world_internal.h"
#include <stddef.h>
#include <string.h>

/* ---------------------------------------------------------------
 * Nibble helpers -- 4-bit values packed 2 per byte (low nibble first).
 * --------------------------------------------------------------- */

static uint8_t nibble_get(const uint8_t *arr, int index)
{
    uint8_t byte = arr[index >> 1];
    return (index & 1) ? (byte >> 4) : (byte & 0x0F);
}

static void nibble_set(uint8_t *arr, int index, uint8_t val)
{
    int byte_idx = index >> 1;
    if (index & 1) {
        arr[byte_idx] = (arr[byte_idx] & 0x0F) | (uint8_t)(val << 4);
    } else {
        arr[byte_idx] = (arr[byte_idx] & 0xF0) | (val & 0x0F);
    }
}

static int block_index(int lx, int ly, int lz)
{
    return ly * 256 + lz * 16 + lx;
}

/* ---------------------------------------------------------------
 * BFS queue -- ring-buffer FIFO.
 * File-scope to avoid stack overflow (~768 KB).
 * --------------------------------------------------------------- */

typedef struct {
    int16_t x, y, z;
    uint8_t level;
} light_node_t;

#define LIGHT_QUEUE_CAP (CHUNK_SIZE_X * CHUNK_SIZE_Z * CHUNK_SIZE_Y * SECTION_COUNT)

typedef struct {
    light_node_t nodes[LIGHT_QUEUE_CAP];
    int          head;
    int          tail;
} light_queue_t;

static light_queue_t g_queue;

static void queue_init(void)
{
    g_queue.head = 0;
    g_queue.tail = 0;
}

static int queue_empty(void)
{
    return g_queue.head == g_queue.tail;
}

static void queue_push(int16_t x, int16_t y, int16_t z, uint8_t level)
{
    light_node_t *n = &g_queue.nodes[g_queue.tail];
    n->x     = x;
    n->y     = y;
    n->z     = z;
    n->level = level;
    g_queue.tail = (g_queue.tail + 1) % LIGHT_QUEUE_CAP;
}

static light_node_t queue_pop(void)
{
    light_node_t n = g_queue.nodes[g_queue.head];
    g_queue.head = (g_queue.head + 1) % LIGHT_QUEUE_CAP;
    return n;
}

/* ---------------------------------------------------------------
 * Shared constants and chunk accessors.
 * --------------------------------------------------------------- */

static const int dx[] = {1, -1, 0, 0, 0, 0};
static const int dy[] = {0, 0, 1, -1, 0, 0};
static const int dz[] = {0, 0, 0, 0, 1, -1};

#define MAX_WORLD_Y (SECTION_COUNT * CHUNK_SIZE_Y)

static block_id_t chunk_block_at(const chunk_t *c, int lx, int world_y, int lz)
{
    int sy = world_y >> 4;
    if (sy < 0 || sy >= SECTION_COUNT) {
        return BLOCK_AIR;
    }
    int ly = world_y & 15;
    return c->sections[sy].blocks[block_index(lx, ly, lz)];
}

/* ---------------------------------------------------------------
 * Unified BFS flood-fill.
 *
 * Drains the global queue, spreading light into transparent
 * neighbors. The caller chooses which nibble array (sky_light or
 * block_light) to read/write via the offset parameter, which is
 * the byte offset of the target array inside chunk_section_t.
 * --------------------------------------------------------------- */

static void bfs_flood(chunk_t *c, size_t array_offset)
{
    while (!queue_empty()) {
        light_node_t nd = queue_pop();
        if (nd.level <= 1) {
            continue;
        }
        uint8_t new_level = nd.level - 1;

        for (int d = 0; d < 6; d++) {
            int nx = nd.x + dx[d];
            int ny = nd.y + dy[d];
            int nz = nd.z + dz[d];

            if (nx < 0 || nx >= CHUNK_SIZE_X ||
                nz < 0 || nz >= CHUNK_SIZE_Z ||
                ny < 0 || ny >= MAX_WORLD_Y) {
                continue;
            }

            block_id_t bid = chunk_block_at(c, nx, ny, nz);
            if (!mc_block_is_transparent(bid)) {
                continue;
            }

            int sy = ny >> 4;
            int ly = ny & 15;
            uint8_t *arr = (uint8_t *)&c->sections[sy] + array_offset;
            int idx = block_index(nx, ly, nz);

            if (nibble_get(arr, idx) >= new_level) {
                continue;
            }
            nibble_set(arr, idx, new_level);
            queue_push((int16_t)nx, (int16_t)ny, (int16_t)nz, new_level);
        }
    }
}

/* ---------------------------------------------------------------
 * Sky light propagation.
 *
 * Column pass: full sunlight (15) falls straight down through
 * transparent blocks. Opaque blocks stop it.
 * BFS pass: seeds sky-lit blocks so light spreads horizontally
 * into shadowed areas (decreases by 1 per step).
 * --------------------------------------------------------------- */

static void propagate_sky(chunk_t *c)
{
    size_t sky_offset = offsetof(chunk_section_t, sky_light);

    for (int sy = 0; sy < SECTION_COUNT; sy++) {
        memset(c->sections[sy].sky_light, 0, sizeof(c->sections[sy].sky_light));
    }

    queue_init();

    for (int lz = 0; lz < CHUNK_SIZE_Z; lz++) {
        for (int lx = 0; lx < CHUNK_SIZE_X; lx++) {
            for (int wy = MAX_WORLD_Y - 1; wy >= 0; wy--) {
                block_id_t bid = chunk_block_at(c, lx, wy, lz);
                if (!mc_block_is_transparent(bid)) {
                    break;
                }
                int sy = wy >> 4;
                int ly = wy & 15;
                nibble_set(c->sections[sy].sky_light, block_index(lx, ly, lz), 15);
                queue_push((int16_t)lx, (int16_t)wy, (int16_t)lz, 15);
            }
        }
    }

    bfs_flood(c, sky_offset);
}

/* ---------------------------------------------------------------
 * Block light propagation.
 *
 * Scans for emitters, seeds them, then BFS flood-fills.
 * Skips sections with no non-air blocks (no possible emitters).
 * --------------------------------------------------------------- */

static void propagate_block_light(chunk_t *c)
{
    size_t bl_offset = offsetof(chunk_section_t, block_light);

    for (int sy = 0; sy < SECTION_COUNT; sy++) {
        memset(c->sections[sy].block_light, 0,
               sizeof(c->sections[sy].block_light));
    }

    queue_init();

    for (int sy = 0; sy < SECTION_COUNT; sy++) {
        if (c->sections[sy].non_air_count == 0) {
            continue;
        }
        for (int ly = 0; ly < CHUNK_SIZE_Y; ly++) {
            for (int lz = 0; lz < CHUNK_SIZE_Z; lz++) {
                for (int lx = 0; lx < CHUNK_SIZE_X; lx++) {
                    block_id_t bid = c->sections[sy].blocks[block_index(lx, ly, lz)];
                    uint8_t emit = mc_block_get_light(bid);
                    if (emit == 0) {
                        continue;
                    }
                    nibble_set(c->sections[sy].block_light,
                               block_index(lx, ly, lz), emit);
                    int wy = sy * CHUNK_SIZE_Y + ly;
                    queue_push((int16_t)lx, (int16_t)wy, (int16_t)lz, emit);
                }
            }
        }
    }

    bfs_flood(c, bl_offset);
}

/* ---------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------- */

void mc_world_propagate_light(chunk_pos_t chunk)
{
    chunk_t *c = world_find_chunk(chunk);
    if (!c) {
        return;
    }
    propagate_sky(c);
    propagate_block_light(c);
}

uint8_t mc_world_get_sky_light(block_pos_t pos)
{
    int32_t chunk_x   = pos.x >> 4;
    int32_t chunk_z   = pos.z >> 4;
    int32_t section_y = pos.y >> 4;

    if (section_y < 0 || section_y >= SECTION_COUNT) {
        return 0;
    }

    chunk_pos_t cpos = {chunk_x, chunk_z};
    const chunk_t *chunk = world_find_chunk(cpos);
    if (!chunk) {
        return 0;
    }

    int32_t lx = pos.x & 15;
    int32_t ly = pos.y & 15;
    int32_t lz = pos.z & 15;
    return nibble_get(chunk->sections[section_y].sky_light,
                      block_index(lx, ly, lz));
}

uint8_t mc_world_get_block_light(block_pos_t pos)
{
    int32_t chunk_x   = pos.x >> 4;
    int32_t chunk_z   = pos.z >> 4;
    int32_t section_y = pos.y >> 4;

    if (section_y < 0 || section_y >= SECTION_COUNT) {
        return 0;
    }

    chunk_pos_t cpos = {chunk_x, chunk_z};
    const chunk_t *chunk = world_find_chunk(cpos);
    if (!chunk) {
        return 0;
    }

    int32_t lx = pos.x & 15;
    int32_t ly = pos.y & 15;
    int32_t lz = pos.z & 15;
    return nibble_get(chunk->sections[section_y].block_light,
                      block_index(lx, ly, lz));
}
