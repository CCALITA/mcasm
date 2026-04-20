#ifndef MC_TYPES_H
#define MC_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef uint16_t block_id_t;
typedef uint32_t entity_id_t;
typedef uint64_t chunk_key_t;
typedef uint32_t tick_t;

typedef struct __attribute__((aligned(16))) {
    float x, y, z, _pad;
} vec3_t;

typedef struct __attribute__((aligned(16))) {
    float x, y, z, w;
} vec4_t;

typedef struct __attribute__((aligned(64))) {
    float m[16];
} mat4_t;

typedef struct __attribute__((aligned(16))) {
    vec3_t min;
    vec3_t max;
} aabb_t;

typedef struct {
    float x, y, z, w;
} quat_t;

typedef struct {
    int32_t x;
    int32_t z;
} chunk_pos_t;

typedef struct {
    int32_t x, y, z, _pad;
} block_pos_t;

#define CHUNK_SIZE_X   16
#define CHUNK_SIZE_Y   16
#define CHUNK_SIZE_Z   16
#define SECTION_COUNT  24
#define BLOCKS_PER_SECTION (CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z)

typedef struct {
    block_id_t blocks[BLOCKS_PER_SECTION];
    uint8_t    block_light[BLOCKS_PER_SECTION / 2];
    uint8_t    sky_light[BLOCKS_PER_SECTION / 2];
    uint16_t   non_air_count;
} chunk_section_t;

typedef struct {
    chunk_pos_t     pos;
    chunk_section_t sections[SECTION_COUNT];
    int32_t         heightmap[CHUNK_SIZE_X * CHUNK_SIZE_Z];
    uint32_t        dirty_mask;
    uint8_t         state;
} chunk_t;

#define CHUNK_STATE_EMPTY       0
#define CHUNK_STATE_GENERATING  1
#define CHUNK_STATE_LIGHTING    2
#define CHUNK_STATE_READY       3
#define CHUNK_STATE_DIRTY       4

typedef struct __attribute__((aligned(16))) {
    vec3_t position;
    vec3_t velocity;
    float  yaw;
    float  pitch;
} transform_component_t;

typedef struct {
    float  max_health;
    float  current_health;
    tick_t invulnerable_until;
} health_component_t;

typedef struct {
    aabb_t  bounding_box;
    uint8_t on_ground;
    uint8_t in_water;
    uint8_t in_lava;
    uint8_t _pad;
} physics_component_t;

typedef struct __attribute__((packed)) {
    uint32_t position_packed;
    uint32_t lighting_packed;
} chunk_vertex_t;

typedef struct {
    chunk_vertex_t* vertices;
    uint32_t        vertex_count;
    uint32_t*       indices;
    uint32_t        index_count;
    chunk_pos_t     chunk_pos;
    uint8_t         section_y;
} chunk_mesh_t;

typedef struct {
    block_pos_t block_pos;
    vec3_t      hit_point;
    vec3_t      hit_normal;
    float       distance;
    uint8_t     hit;
} raycast_result_t;

#endif /* MC_TYPES_H */
