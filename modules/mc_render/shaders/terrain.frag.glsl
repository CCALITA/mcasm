#version 450

layout(set = 0, binding = 0) uniform sampler2D atlas_tex;

layout(location = 0) in float frag_light;
layout(location = 1) in vec2  frag_uv;
layout(location = 0) out vec4 out_color;

void main() {
    vec4 tex_color = texture(atlas_tex, frag_uv);
    out_color = vec4(tex_color.rgb * frag_light, tex_color.a);
}
