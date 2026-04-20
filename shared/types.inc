; types.inc — NASM struct definitions mirroring include/mc_types.h
; Keep in sync with C headers. CI validates via toolchain/ci/abi-check.py

%include "platform.inc"

%define SIZEOF_VEC3         16
%define SIZEOF_VEC4         16
%define SIZEOF_MAT4         64
%define SIZEOF_AABB         32

%define VEC3_X              0
%define VEC3_Y              4
%define VEC3_Z              8

%define VEC4_X              0
%define VEC4_Y              4
%define VEC4_Z              8
%define VEC4_W              12

%define MAT4_DATA           0

%define AABB_MIN            0
%define AABB_MAX            16

%define CHUNK_SIZE_X        16
%define CHUNK_SIZE_Y        16
%define CHUNK_SIZE_Z        16
%define SECTION_COUNT       24
%define BLOCKS_PER_SECTION  4096

%define CSECT_BLOCKS        0
%define CSECT_BLOCK_LIGHT   8192
%define CSECT_SKY_LIGHT     10240
%define CSECT_NONAIR        12288
%define SIZEOF_CHUNK_SECTION 12292

%define CVERT_POS_PACKED    0
%define CVERT_LIGHT_PACKED  4
%define SIZEOF_CHUNK_VERTEX 8

%define XFORM_POS           0
%define XFORM_VEL           16
%define XFORM_YAW           32
%define XFORM_PITCH         36
%define SIZEOF_XFORM        48

%define COMPONENT_TRANSFORM  (1 << 0)
%define COMPONENT_PHYSICS    (1 << 1)
%define COMPONENT_RENDER     (1 << 2)
%define COMPONENT_HEALTH     (1 << 3)
%define COMPONENT_AI         (1 << 4)
%define COMPONENT_INVENTORY  (1 << 5)
%define COMPONENT_PLAYER     (1 << 6)

%define MAX_ENTITIES         4096
%define ENTITY_INVALID       0
