#version 450

layout(location = 0) in float frag_light;
layout(location = 0) out vec4 out_color;

void main() {
    vec3 water_base = vec3(0.2, 0.4, 0.8);
    float darkening = mix(0.6, 1.0, frag_light);
    out_color = vec4(water_base * darkening, 0.6);
}
