#include "mc_math.h"
#include <math.h>
#include <string.h>

vec3_t mc_math_vec3_add(vec3_t a, vec3_t b) { return (vec3_t){a.x+b.x, a.y+b.y, a.z+b.z}; }
vec3_t mc_math_vec3_sub(vec3_t a, vec3_t b) { return (vec3_t){a.x-b.x, a.y-b.y, a.z-b.z}; }
vec3_t mc_math_vec3_scale(vec3_t v, float s) { return (vec3_t){v.x*s, v.y*s, v.z*s}; }
float mc_math_vec3_dot(vec3_t a, vec3_t b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
vec3_t mc_math_vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
float mc_math_vec3_length(vec3_t v) { return sqrtf(mc_math_vec3_dot(v, v)); }
vec3_t mc_math_vec3_normalize(vec3_t v) { float l = mc_math_vec3_length(v); return l > 0 ? mc_math_vec3_scale(v, 1.0f/l) : v; }

vec4_t mc_math_vec4_add(vec4_t a, vec4_t b) { return (vec4_t){a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w}; }
float mc_math_vec4_dot(vec4_t a, vec4_t b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

mat4_t mc_math_mat4_identity(void) {
    mat4_t m = {0}; m.m[0]=m.m[5]=m.m[10]=m.m[15]=1.0f; return m;
}
mat4_t mc_math_mat4_multiply(mat4_t a, mat4_t b) {
    mat4_t r = {0};
    for (int c=0;c<4;c++) for (int row=0;row<4;row++) for (int k=0;k<4;k++)
        r.m[c*4+row] += a.m[k*4+row] * b.m[c*4+k];
    return r;
}
mat4_t mc_math_mat4_translate(vec3_t offset) {
    mat4_t m = mc_math_mat4_identity(); m.m[12]=offset.x; m.m[13]=offset.y; m.m[14]=offset.z; return m;
}
mat4_t mc_math_mat4_rotate_x(float rad) {
    mat4_t m = mc_math_mat4_identity(); m.m[5]=cosf(rad); m.m[6]=sinf(rad); m.m[9]=-sinf(rad); m.m[10]=cosf(rad); return m;
}
mat4_t mc_math_mat4_rotate_y(float rad) {
    mat4_t m = mc_math_mat4_identity(); m.m[0]=cosf(rad); m.m[2]=-sinf(rad); m.m[8]=sinf(rad); m.m[10]=cosf(rad); return m;
}
mat4_t mc_math_mat4_perspective(float fov, float aspect, float near, float far) {
    mat4_t m = {0}; float f = 1.0f/tanf(fov/2.0f);
    m.m[0]=f/aspect; m.m[5]=f; m.m[10]=(far+near)/(near-far); m.m[11]=-1; m.m[14]=2*far*near/(near-far); return m;
}
mat4_t mc_math_mat4_look_at(vec3_t eye, vec3_t center, vec3_t up) {
    vec3_t f = mc_math_vec3_normalize(mc_math_vec3_sub(center, eye));
    vec3_t s = mc_math_vec3_normalize(mc_math_vec3_cross(f, up));
    vec3_t u = mc_math_vec3_cross(s, f);
    mat4_t m = mc_math_mat4_identity();
    m.m[0]=s.x; m.m[4]=s.y; m.m[8]=s.z;
    m.m[1]=u.x; m.m[5]=u.y; m.m[9]=u.z;
    m.m[2]=-f.x; m.m[6]=-f.y; m.m[10]=-f.z;
    m.m[12]=-mc_math_vec3_dot(s,eye); m.m[13]=-mc_math_vec3_dot(u,eye); m.m[14]=mc_math_vec3_dot(f,eye);
    return m;
}
vec4_t mc_math_mat4_transform(mat4_t m, vec4_t v) {
    return (vec4_t){
        m.m[0]*v.x+m.m[4]*v.y+m.m[8]*v.z+m.m[12]*v.w,
        m.m[1]*v.x+m.m[5]*v.y+m.m[9]*v.z+m.m[13]*v.w,
        m.m[2]*v.x+m.m[6]*v.y+m.m[10]*v.z+m.m[14]*v.w,
        m.m[3]*v.x+m.m[7]*v.y+m.m[11]*v.z+m.m[15]*v.w
    };
}
quat_t mc_math_quat_from_euler(float yaw, float pitch, float roll) {
    (void)yaw; (void)pitch; (void)roll; return (quat_t){0,0,0,1};
}
vec3_t mc_math_quat_rotate(quat_t q, vec3_t v) { (void)q; return v; }
uint8_t mc_math_aabb_intersects(aabb_t a, aabb_t b) {
    return a.min.x<=b.max.x && a.max.x>=b.min.x && a.min.y<=b.max.y && a.max.y>=b.min.y && a.min.z<=b.max.z && a.max.z>=b.min.z;
}
uint8_t mc_math_aabb_contains(aabb_t box, vec3_t p) {
    return p.x>=box.min.x && p.x<=box.max.x && p.y>=box.min.y && p.y<=box.max.y && p.z>=box.min.z && p.z<=box.max.z;
}
float mc_math_noise_perlin2d(float x, float y, uint32_t seed) { (void)x;(void)y;(void)seed; return 0.0f; }
float mc_math_noise_perlin3d(float x, float y, float z, uint32_t seed) { (void)x;(void)y;(void)z;(void)seed; return 0.0f; }
float mc_math_noise_simplex2d(float x, float y, uint32_t seed) { (void)x;(void)y;(void)seed; return 0.0f; }
