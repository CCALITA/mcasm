#include "render_internal.h"
#include "mc_render.h"
#include <stdlib.h>
#include <string.h>

/*
 * Chunk meshing: iterate over blocks in a section and emit quads for
 * exposed faces of non-air blocks. Each face has 6 vertices (2 triangles)
 * forming a quad, indexed with an index buffer.
 *
 * Vertex packing (matches terrain.vert.glsl):
 *   position_packed:  bits [4:0]=x, [13:5]=y, [18:14]=z  (5+9+5 = 19 bits)
 *   lighting_packed:  bits [3:0]=light level (0-15)
 */

static inline uint32_t block_index(int x, int y, int z)
{
    return (uint32_t)(y * CHUNK_SIZE_X * CHUNK_SIZE_Z + z * CHUNK_SIZE_X + x);
}

static inline uint32_t pack_position(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & 0x1Fu) | ((y & 0x1FFu) << 5) | ((z & 0x1Fu) << 14);
}

static inline uint32_t pack_lighting(uint8_t light)
{
    return (uint32_t)(light & 0xFu);
}

/* 4-bit values, packed 2 per byte */
static inline uint8_t get_sky_light(const chunk_section_t *s, int x, int y, int z)
{
    uint32_t idx = block_index(x, y, z);
    uint8_t byte_val = s->sky_light[idx / 2];
    return (idx & 1) ? (byte_val >> 4) : (byte_val & 0x0F);
}

typedef struct {
    int dx, dy, dz;
} face_dir_t;

/* +X, -X, +Y, -Y, +Z, -Z */
static const face_dir_t FACE_DIRS[6] = {
    { 1,  0,  0},
    {-1,  0,  0},
    { 0,  1,  0},
    { 0, -1,  0},
    { 0,  0,  1},
    { 0,  0, -1},
};

/*
 * Quad vertex offsets for each face (4 corners, CCW winding).
 * Each row: {offset_x, offset_y, offset_z} relative to block origin.
 */
static const int FACE_VERTS[6][4][3] = {
    /* +X face (x=1 plane) */
    {{1,0,0}, {1,1,0}, {1,1,1}, {1,0,1}},
    /* -X face (x=0 plane) */
    {{0,0,1}, {0,1,1}, {0,1,0}, {0,0,0}},
    /* +Y face (y=1 plane) */
    {{0,1,0}, {0,1,1}, {1,1,1}, {1,1,0}},
    /* -Y face (y=0 plane) */
    {{0,0,1}, {0,0,0}, {1,0,0}, {1,0,1}},
    /* +Z face (z=1 plane) */
    {{1,0,1}, {1,1,1}, {0,1,1}, {0,0,1}},
    /* -Z face (z=0 plane) */
    {{0,0,0}, {0,1,0}, {1,1,0}, {1,0,0}},
};

/* Index pattern for one quad: 2 triangles, 6 indices */
static const uint32_t QUAD_INDICES[6] = {0, 1, 2, 0, 2, 3};

chunk_mesh_t *build_chunk_mesh_data(const chunk_section_t *section,
                                    chunk_pos_t chunk_pos, uint8_t section_y)
{
    if (!section || section->non_air_count == 0) return NULL;

    /* Worst case: every non-air block has 6 faces, 4 verts + 6 indices each */
    uint32_t max_verts   = section->non_air_count * 6 * 4;
    uint32_t max_indices = section->non_air_count * 6 * 6;

    chunk_vertex_t *vertices = calloc(max_verts, sizeof(chunk_vertex_t));
    uint32_t       *indices  = calloc(max_indices, sizeof(uint32_t));
    if (!vertices || !indices) {
        free(vertices);
        free(indices);
        return NULL;
    }

    uint32_t vert_count = 0;
    uint32_t idx_count  = 0;

    for (int y = 0; y < CHUNK_SIZE_Y; y++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            for (int x = 0; x < CHUNK_SIZE_X; x++) {
                block_id_t block = section->blocks[block_index(x, y, z)];
                if (block == 0) continue; /* air */

                uint8_t light = get_sky_light(section, x, y, z);
                if (light == 0) light = 8; /* default light for solid blocks */

                for (int face = 0; face < 6; face++) {
                    int nx = x + FACE_DIRS[face].dx;
                    int ny = y + FACE_DIRS[face].dy;
                    int nz = z + FACE_DIRS[face].dz;

                    /* If neighbor is inside the section, check if it's air */
                    if (nx >= 0 && nx < CHUNK_SIZE_X &&
                        ny >= 0 && ny < CHUNK_SIZE_Y &&
                        nz >= 0 && nz < CHUNK_SIZE_Z) {
                        block_id_t neighbor = section->blocks[block_index(nx, ny, nz)];
                        if (neighbor != 0) continue; /* occluded */
                    }
                    /* Boundary faces are always emitted (neighbor unknown) */

                    /* Emit 4 vertices for this quad */
                    uint32_t base_vert = vert_count;
                    uint32_t world_y = (uint32_t)section_y * CHUNK_SIZE_Y + (uint32_t)y;

                    for (int v = 0; v < 4; v++) {
                        uint32_t vx = (uint32_t)x + (uint32_t)FACE_VERTS[face][v][0];
                        uint32_t vy = world_y    + (uint32_t)FACE_VERTS[face][v][1];
                        uint32_t vz = (uint32_t)z + (uint32_t)FACE_VERTS[face][v][2];

                        vertices[vert_count].position_packed = pack_position(vx, vy, vz);
                        vertices[vert_count].lighting_packed = pack_lighting(light);
                        vert_count++;
                    }

                    /* Emit 6 indices for 2 triangles */
                    for (int i = 0; i < 6; i++) {
                        indices[idx_count++] = base_vert + QUAD_INDICES[i];
                    }
                }
            }
        }
    }

    if (vert_count == 0) {
        free(vertices);
        free(indices);
        return NULL;
    }

    /* Allocate and fill chunk_mesh_t */
    chunk_mesh_t *mesh = calloc(1, sizeof(chunk_mesh_t));
    if (!mesh) {
        free(vertices);
        free(indices);
        return NULL;
    }

    /* Shrink allocations to actual size */
    chunk_vertex_t *final_verts = realloc(vertices, vert_count * sizeof(chunk_vertex_t));
    uint32_t       *final_idx   = realloc(indices, idx_count * sizeof(uint32_t));
    if (!final_verts) final_verts = vertices;
    if (!final_idx)   final_idx   = indices;

    mesh->vertices     = final_verts;
    mesh->vertex_count = vert_count;
    mesh->indices      = final_idx;
    mesh->index_count  = idx_count;
    mesh->chunk_pos    = chunk_pos;
    mesh->section_y    = section_y;

    return mesh;
}

uint64_t mc_render_build_chunk_mesh(const chunk_section_t *section,
                                    chunk_pos_t chunk_pos, uint8_t section_y)
{
    chunk_mesh_t *mesh = build_chunk_mesh_data(section, chunk_pos, section_y);
    if (!mesh) return 0;

    uint64_t handle = mc_render_upload_mesh(mesh);

    free(mesh->vertices);
    free(mesh->indices);
    free(mesh);

    return handle;
}
