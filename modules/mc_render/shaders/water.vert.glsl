#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    float time;
} pc;

layout(location = 0) in uint packed_pos;
layout(location = 1) in uint packed_light;
layout(location = 0) out float frag_light;

void main() {
    float x = float(packed_pos & 0x1Fu);
    float y = float((packed_pos >> 5) & 0x1FFu);
    float z = float((packed_pos >> 14) & 0x1Fu);

    float y_offset = sin(pc.time + x * 0.5 + z * 0.5) * 0.05;

    gl_Position = pc.mvp * vec4(x, y + y_offset, z, 1.0);
    frag_light = float(packed_light & 0xFu) / 15.0;
}
