#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 chunk_offset;
} pc;

layout(location = 0) in uint packed_pos;
layout(location = 1) in uint packed_light;
layout(location = 0) out float frag_light;
layout(location = 1) out vec2  frag_uv;

void main() {
    float x = float(packed_pos & 0x1Fu) + pc.chunk_offset.x;
    float y = float((packed_pos >> 5) & 0x1FFu) + pc.chunk_offset.y;
    float z = float((packed_pos >> 14) & 0x1Fu) + pc.chunk_offset.z;
    gl_Position = pc.mvp * vec4(x, y, z, 1.0);

    float light = float(packed_light & 0xFu) / 15.0;
    frag_light = max(light, 0.15);

    uint tex_index = (packed_light >> 4) & 0xFFu;
    uint uv_corner = (packed_light >> 12) & 0x3u;

    float tile_x = float(tex_index % 16u);
    float tile_y = float(tex_index / 16u);

    float u_off = ((uv_corner == 2u) || (uv_corner == 3u)) ? 1.0 : 0.0;
    float v_off = ((uv_corner == 1u) || (uv_corner == 2u)) ? 1.0 : 0.0;

    frag_uv = (vec2(tile_x, tile_y) + vec2(u_off, v_off)) / 16.0;
}
