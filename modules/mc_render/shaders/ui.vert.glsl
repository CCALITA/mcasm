#version 450

layout(location = 0) in vec2 in_pos;  /* NDC x,y from UI module */
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec2 frag_uv;

void main() {
    gl_Position = vec4(in_pos, 0.0, 1.0);
    frag_uv     = in_uv;
}
