#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

layout(location = 0) in uint packed_pos;
layout(location = 1) in uint packed_light;
layout(location = 0) out vec3 frag_color;

void main() {
    float x = float(packed_pos & 0x1Fu);
    float y = float((packed_pos >> 5) & 0x1FFu);
    float z = float((packed_pos >> 14) & 0x1Fu);
    gl_Position = pc.mvp * vec4(x, y, z, 1.0);
    float light = float(packed_light & 0xFu) / 15.0;
    frag_color = vec3(light);
}
