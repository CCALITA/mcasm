#version 450

layout(location = 0) in vec2 frag_uv;
layout(location = 0) out vec4 out_color;

void main() {
    /* No texture bound yet -- render all UI as solid white.
       When a font/icon atlas is added, sample it here. */
    out_color = vec4(1.0, 1.0, 1.0, 1.0);
}
