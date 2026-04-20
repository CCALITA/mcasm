#ifndef MC_MATH_H
#define MC_MATH_H

#include "mc_types.h"

vec3_t mc_math_vec3_add(vec3_t a, vec3_t b);
vec3_t mc_math_vec3_sub(vec3_t a, vec3_t b);
vec3_t mc_math_vec3_scale(vec3_t v, float s);
float  mc_math_vec3_dot(vec3_t a, vec3_t b);
vec3_t mc_math_vec3_cross(vec3_t a, vec3_t b);
float  mc_math_vec3_length(vec3_t v);
vec3_t mc_math_vec3_normalize(vec3_t v);

vec4_t mc_math_vec4_add(vec4_t a, vec4_t b);
float  mc_math_vec4_dot(vec4_t a, vec4_t b);

mat4_t mc_math_mat4_identity(void);
mat4_t mc_math_mat4_multiply(mat4_t a, mat4_t b);
mat4_t mc_math_mat4_translate(vec3_t offset);
mat4_t mc_math_mat4_rotate_x(float radians);
mat4_t mc_math_mat4_rotate_y(float radians);
mat4_t mc_math_mat4_perspective(float fov, float aspect, float near, float far);
mat4_t mc_math_mat4_look_at(vec3_t eye, vec3_t center, vec3_t up);
vec4_t mc_math_mat4_transform(mat4_t m, vec4_t v);

quat_t mc_math_quat_from_euler(float yaw, float pitch, float roll);
vec3_t mc_math_quat_rotate(quat_t q, vec3_t v);

uint8_t mc_math_aabb_intersects(aabb_t a, aabb_t b);
uint8_t mc_math_aabb_contains(aabb_t box, vec3_t point);

float mc_math_noise_perlin2d(float x, float y, uint32_t seed);
float mc_math_noise_perlin3d(float x, float y, float z, uint32_t seed);
float mc_math_noise_simplex2d(float x, float y, uint32_t seed);

#endif /* MC_MATH_H */
