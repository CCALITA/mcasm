#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

layout(location = 0) in uint packed_pos;
layout(location = 1) in uint packed_light;
layout(location = 0) out float frag_light;
layout(location = 1) out vec2  frag_uv;

void main() {
    float x = float(packed_pos & 0x1Fu);
    float y = float((packed_pos >> 5) & 0x1FFu);
    float z = float((packed_pos >> 14) & 0x1Fu);
    gl_Position = pc.mvp * vec4(x, y, z, 1.0);

    float light = float(packed_light & 0xFu) / 15.0;
    frag_light = light;

    /* Decode texture index (bits 11:4) and UV corner (bits 13:12) */
    uint tex_index = (packed_light >> 4) & 0xFFu;
    uint uv_corner = (packed_light >> 12) & 0x3u;

    /* Tile position in the 16x16 grid */
    float tile_x = float(tex_index % 16u);
    float tile_y = float(tex_index / 16u);

    /* UV corner: 0=(0,0), 1=(0,1), 2=(1,1), 3=(1,0) */
    float u_off = ((uv_corner == 2u) || (uv_corner == 3u)) ? 1.0 : 0.0;
    float v_off = ((uv_corner == 1u) || (uv_corner == 2u)) ? 1.0 : 0.0;

    frag_uv = (vec2(tile_x, tile_y) + vec2(u_off, v_off)) / 16.0;
}
