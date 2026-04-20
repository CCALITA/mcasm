#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mc_types.h"
#include "mc_error.h"
#include "mc_block.h"

#include "render_internal.h"

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s (line %d): %s\n", __func__, __LINE__, msg); \
        return 1; \
    } \
} while (0)

/* Test: a single solid block at (0,0,0) in an otherwise empty section
 * should produce 6 faces = 24 vertices, 36 indices */
static int test_single_block_mesh(void)
{
    chunk_section_t section;
    memset(&section, 0, sizeof(section));

    /* Place one stone block at (0,0,0) */
    section.blocks[0] = 1; /* block ID 1 = stone */
    section.non_air_count = 1;

    /* Set some sky light */
    section.sky_light[0] = 0x0F; /* block 0 gets light=15, block 1 gets light=0 */

    chunk_pos_t cpos = {0, 0};
    chunk_mesh_t *mesh = build_chunk_mesh_data(&section, cpos, 0);

    ASSERT(mesh != NULL, "mesh should not be NULL for single solid block");
    ASSERT(mesh->vertex_count == 24, "single block should have 24 vertices (6 faces * 4 verts)");
    ASSERT(mesh->index_count == 36, "single block should have 36 indices (6 faces * 6 indices)");
    ASSERT(mesh->vertices != NULL, "vertices pointer should not be NULL");
    ASSERT(mesh->indices != NULL, "indices pointer should not be NULL");

    /* Verify position packing for at least one vertex */
    /* Block (0,0,0) with section_y=0: world_y=0 */
    /* First face is +X: verts at (1,0,0),(1,1,0),(1,1,1),(1,0,1) */
    uint32_t first_pos = mesh->vertices[0].position_packed;
    uint32_t vx = first_pos & 0x1Fu;
    uint32_t vy = (first_pos >> 5) & 0x1FFu;
    uint32_t vz = (first_pos >> 14) & 0x1Fu;
    ASSERT(vx == 1, "first vertex x should be 1 (+X face)");
    ASSERT(vy == 0, "first vertex y should be 0");
    ASSERT(vz == 0, "first vertex z should be 0");

    /* Verify lighting is packed correctly */
    uint32_t light_packed = mesh->vertices[0].lighting_packed;
    uint32_t light_val = light_packed & 0xFu;
    ASSERT(light_val == 15, "sky light should be 15 for first block");

    free(mesh->vertices);
    free(mesh->indices);
    free(mesh);

    printf("  PASS: test_single_block_mesh\n");
    return 0;
}

/* Test: empty section (non_air_count=0) should produce NULL mesh */
static int test_empty_section_mesh(void)
{
    chunk_section_t section;
    memset(&section, 0, sizeof(section));
    section.non_air_count = 0;

    chunk_pos_t cpos = {0, 0};
    chunk_mesh_t *mesh = build_chunk_mesh_data(&section, cpos, 0);

    ASSERT(mesh == NULL, "empty section should produce NULL mesh");

    printf("  PASS: test_empty_section_mesh\n");
    return 0;
}

/* Test: two adjacent blocks should not emit the shared face */
static int test_adjacent_blocks_occlusion(void)
{
    chunk_section_t section;
    memset(&section, 0, sizeof(section));

    /* Place two blocks adjacent in X: (0,0,0) and (1,0,0) */
    section.blocks[0] = 1; /* (0,0,0) */
    section.blocks[1] = 1; /* (1,0,0) */
    section.non_air_count = 2;

    chunk_pos_t cpos = {0, 0};
    chunk_mesh_t *mesh = build_chunk_mesh_data(&section, cpos, 0);

    ASSERT(mesh != NULL, "mesh should not be NULL for two blocks");

    /*
     * Two isolated blocks would each have 6 faces = 12 faces total.
     * But they share one face: block0's +X is adjacent to block1's -X.
     * So 2 faces are occluded, leaving 10 faces.
     * 10 faces * 4 verts = 40 vertices, 10 * 6 = 60 indices.
     */
    ASSERT(mesh->vertex_count == 40, "two adjacent blocks should have 40 vertices (10 faces * 4)");
    ASSERT(mesh->index_count == 60, "two adjacent blocks should have 60 indices (10 faces * 6)");

    free(mesh->vertices);
    free(mesh->indices);
    free(mesh);

    printf("  PASS: test_adjacent_blocks_occlusion\n");
    return 0;
}

/* Test: NULL section pointer */
static int test_null_section(void)
{
    chunk_mesh_t *mesh = build_chunk_mesh_data(NULL, (chunk_pos_t){0, 0}, 0);
    ASSERT(mesh == NULL, "NULL section should produce NULL mesh");

    printf("  PASS: test_null_section\n");
    return 0;
}

/* Test: section_y offset affects world Y in packed position */
static int test_section_y_offset(void)
{
    chunk_section_t section;
    memset(&section, 0, sizeof(section));

    section.blocks[0] = 1;
    section.non_air_count = 1;

    chunk_pos_t cpos = {0, 0};
    uint8_t section_y = 3;
    chunk_mesh_t *mesh = build_chunk_mesh_data(&section, cpos, section_y);

    ASSERT(mesh != NULL, "mesh should not be NULL");

    /* Block at local y=0, section_y=3 => world_y = 3*16 = 48 */
    /* First face (+X), first vertex (1,0,0) => world coords (1, 48, 0) */
    uint32_t pos = mesh->vertices[0].position_packed;
    uint32_t vy = (pos >> 5) & 0x1FFu;
    ASSERT(vy == 48, "world y should be section_y * 16 = 48");

    free(mesh->vertices);
    free(mesh->indices);
    free(mesh);

    printf("  PASS: test_section_y_offset\n");
    return 0;
}

/* Test: water block should not appear in opaque mesh */
static int test_water_excluded_from_opaque(void)
{
    chunk_section_t section;
    memset(&section, 0, sizeof(section));

    /* Place one water block at (0,0,0) */
    section.blocks[0] = BLOCK_WATER;
    section.non_air_count = 1;

    chunk_pos_t cpos = {0, 0};
    chunk_mesh_t *mesh = build_chunk_mesh_data(&section, cpos, 0);

    ASSERT(mesh == NULL, "water-only section should produce NULL opaque mesh");

    printf("  PASS: test_water_excluded_from_opaque\n");
    return 0;
}

/* Test: single water block produces water mesh with 6 faces */
static int test_single_water_block_mesh(void)
{
    chunk_section_t section;
    memset(&section, 0, sizeof(section));

    section.blocks[0] = BLOCK_WATER;
    section.non_air_count = 1;
    section.sky_light[0] = 0x0F;

    chunk_pos_t cpos = {0, 0};
    chunk_mesh_t *mesh = build_water_mesh_data(&section, cpos, 0);

    ASSERT(mesh != NULL, "water mesh should not be NULL for single water block");
    ASSERT(mesh->vertex_count == 24, "single water block: 24 vertices (6 faces * 4)");
    ASSERT(mesh->index_count == 36, "single water block: 36 indices (6 faces * 6)");

    free(mesh->vertices);
    free(mesh->indices);
    free(mesh);

    printf("  PASS: test_single_water_block_mesh\n");
    return 0;
}

/* Test: two adjacent water blocks do not emit the shared face */
static int test_adjacent_water_occlusion(void)
{
    chunk_section_t section;
    memset(&section, 0, sizeof(section));

    section.blocks[0] = BLOCK_WATER; /* (0,0,0) */
    section.blocks[1] = BLOCK_WATER; /* (1,0,0) */
    section.non_air_count = 2;

    chunk_pos_t cpos = {0, 0};
    chunk_mesh_t *mesh = build_water_mesh_data(&section, cpos, 0);

    ASSERT(mesh != NULL, "water mesh should not be NULL for two water blocks");
    ASSERT(mesh->vertex_count == 40, "two adjacent water: 40 verts (10 faces * 4)");
    ASSERT(mesh->index_count == 60, "two adjacent water: 60 indices (10 faces * 6)");

    free(mesh->vertices);
    free(mesh->indices);
    free(mesh);

    printf("  PASS: test_adjacent_water_occlusion\n");
    return 0;
}

/* Test: water next to stone -- water face toward stone is not emitted,
 * but stone face toward water IS emitted in opaque mesh */
static int test_water_stone_boundary(void)
{
    chunk_section_t section;
    memset(&section, 0, sizeof(section));

    /* Stone at (0,0,0), water at (1,0,0) */
    section.blocks[0] = 1;           /* stone */
    section.blocks[1] = BLOCK_WATER; /* water at x=1 */
    section.non_air_count = 2;

    chunk_pos_t cpos = {0, 0};

    /* Opaque mesh: stone exposes all 6 faces (water counts as transparent) */
    chunk_mesh_t *opaque = build_chunk_mesh_data(&section, cpos, 0);
    ASSERT(opaque != NULL, "opaque mesh should not be NULL");
    ASSERT(opaque->vertex_count == 24, "stone with water neighbor: 24 verts (6 faces)");

    free(opaque->vertices);
    free(opaque->indices);
    free(opaque);

    /* Water mesh: water at (1,0,0) has stone neighbor on -X, skip that face.
     * 5 remaining faces emitted. */
    chunk_mesh_t *water = build_water_mesh_data(&section, cpos, 0);
    ASSERT(water != NULL, "water mesh should not be NULL");
    ASSERT(water->vertex_count == 20, "water next to stone: 20 verts (5 faces * 4)");
    ASSERT(water->index_count == 30, "water next to stone: 30 indices (5 faces * 6)");

    free(water->vertices);
    free(water->indices);
    free(water);

    printf("  PASS: test_water_stone_boundary\n");
    return 0;
}

/* Test: section with no water produces NULL water mesh */
static int test_no_water_section(void)
{
    chunk_section_t section;
    memset(&section, 0, sizeof(section));

    section.blocks[0] = 1; /* stone */
    section.non_air_count = 1;

    chunk_pos_t cpos = {0, 0};
    chunk_mesh_t *mesh = build_water_mesh_data(&section, cpos, 0);

    ASSERT(mesh == NULL, "section without water should produce NULL water mesh");

    printf("  PASS: test_no_water_section\n");
    return 0;
}

int main(void)
{
    int failures = 0;
    printf("Running mc_render mesh tests...\n");

    failures += test_single_block_mesh();
    failures += test_empty_section_mesh();
    failures += test_adjacent_blocks_occlusion();
    failures += test_null_section();
    failures += test_section_y_offset();
    failures += test_water_excluded_from_opaque();
    failures += test_single_water_block_mesh();
    failures += test_adjacent_water_occlusion();
    failures += test_water_stone_boundary();
    failures += test_no_water_section();

    if (failures > 0) {
        fprintf(stderr, "%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All mesh tests passed.\n");
    return 0;
}
