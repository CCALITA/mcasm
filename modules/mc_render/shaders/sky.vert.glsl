#version 450

layout(location = 0) out vec2 frag_uv;

void main() {
    /* Generate a fullscreen triangle from vertex ID (0, 1, 2).
     * Vertex 0: (-1, -1)  UV (0, 0)
     * Vertex 1: ( 3, -1)  UV (2, 0)
     * Vertex 2: (-1,  3)  UV (0, 2)
     * The triangle covers the entire clip-space quad [-1,1]x[-1,1]. */
    frag_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(frag_uv * 2.0 - 1.0, 0.9999, 1.0);
}
