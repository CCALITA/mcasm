#include "mc_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define EPSILON 1e-4f

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT_FLOAT_EQ(msg, expected, actual) do { \
    float _e = (expected), _a = (actual); \
    if (fabsf(_e - _a) < EPSILON) { g_pass++; } \
    else { g_fail++; fprintf(stderr, "FAIL %s: expected %.6f, got %.6f\n", msg, _e, _a); } \
} while (0)

static void test_mat4_identity(void) {
    mat4_t m = mc_math_mat4_identity();
    ASSERT_FLOAT_EQ("identity[0]",  1.0f, m.m[0]);
    ASSERT_FLOAT_EQ("identity[1]",  0.0f, m.m[1]);
    ASSERT_FLOAT_EQ("identity[5]",  1.0f, m.m[5]);
    ASSERT_FLOAT_EQ("identity[10]", 1.0f, m.m[10]);
    ASSERT_FLOAT_EQ("identity[15]", 1.0f, m.m[15]);
    ASSERT_FLOAT_EQ("identity[4]",  0.0f, m.m[4]);
    ASSERT_FLOAT_EQ("identity[12]", 0.0f, m.m[12]);
}

static void test_mat4_multiply_identity(void) {
    mat4_t id = mc_math_mat4_identity();
    mat4_t t = mc_math_mat4_translate((vec3_t){3.0f, 4.0f, 5.0f, 0.0f});
    mat4_t r = mc_math_mat4_multiply(t, id);
    /* Multiplying by identity should yield the same matrix */
    for (int i = 0; i < 16; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "mul_id[%d]", i);
        ASSERT_FLOAT_EQ(buf, t.m[i], r.m[i]);
    }
}

static void test_mat4_translate(void) {
    mat4_t t = mc_math_mat4_translate((vec3_t){1.0f, 2.0f, 3.0f, 0.0f});
    /* Translation matrix should have offsets in column 3 */
    ASSERT_FLOAT_EQ("translate[12]", 1.0f, t.m[12]);
    ASSERT_FLOAT_EQ("translate[13]", 2.0f, t.m[13]);
    ASSERT_FLOAT_EQ("translate[14]", 3.0f, t.m[14]);
    ASSERT_FLOAT_EQ("translate[15]", 1.0f, t.m[15]);
    /* Diagonal should be 1 */
    ASSERT_FLOAT_EQ("translate[0]",  1.0f, t.m[0]);
    ASSERT_FLOAT_EQ("translate[5]",  1.0f, t.m[5]);
    ASSERT_FLOAT_EQ("translate[10]", 1.0f, t.m[10]);
}

static void test_mat4_transform(void) {
    /* Translate (1,2,3) applied to point (0,0,0,1) should yield (1,2,3,1) */
    mat4_t t = mc_math_mat4_translate((vec3_t){1.0f, 2.0f, 3.0f, 0.0f});
    vec4_t p = {0.0f, 0.0f, 0.0f, 1.0f};
    vec4_t r = mc_math_mat4_transform(t, p);
    ASSERT_FLOAT_EQ("transform.x", 1.0f, r.x);
    ASSERT_FLOAT_EQ("transform.y", 2.0f, r.y);
    ASSERT_FLOAT_EQ("transform.z", 3.0f, r.z);
    ASSERT_FLOAT_EQ("transform.w", 1.0f, r.w);
}

static void test_mat4_rotate_x(void) {
    /* Rotate 90 degrees around X: (0,1,0) -> (0,0,1) */
    float angle = (float)(M_PI / 2.0);
    mat4_t rx = mc_math_mat4_rotate_x(angle);
    vec4_t v = {0.0f, 1.0f, 0.0f, 0.0f};
    vec4_t r = mc_math_mat4_transform(rx, v);
    ASSERT_FLOAT_EQ("rotate_x.x", 0.0f, r.x);
    ASSERT_FLOAT_EQ("rotate_x.y", 0.0f, r.y);
    ASSERT_FLOAT_EQ("rotate_x.z", 1.0f, r.z);
}

static void test_mat4_rotate_y(void) {
    /* Rotate 90 degrees around Y: (1,0,0) -> (0,0,-1) */
    float angle = (float)(M_PI / 2.0);
    mat4_t ry = mc_math_mat4_rotate_y(angle);
    vec4_t v = {1.0f, 0.0f, 0.0f, 0.0f};
    vec4_t r = mc_math_mat4_transform(ry, v);
    ASSERT_FLOAT_EQ("rotate_y.x", 0.0f, r.x);
    ASSERT_FLOAT_EQ("rotate_y.y", 0.0f, r.y);
    ASSERT_FLOAT_EQ("rotate_y.z", -1.0f, r.z);
}

static void test_mat4_perspective(void) {
    float fov = (float)(M_PI / 4.0); /* 45 degrees */
    float aspect = 16.0f / 9.0f;
    float near = 0.1f;
    float far = 100.0f;
    mat4_t p = mc_math_mat4_perspective(fov, aspect, near, far);

    float f = 1.0f / tanf(fov / 2.0f);
    ASSERT_FLOAT_EQ("persp[0]",  f / aspect, p.m[0]);
    ASSERT_FLOAT_EQ("persp[5]",  f,          p.m[5]);
    ASSERT_FLOAT_EQ("persp[11]", -1.0f,      p.m[11]);
    ASSERT_FLOAT_EQ("persp[10]", (far + near) / (near - far), p.m[10]);
    ASSERT_FLOAT_EQ("persp[14]", 2.0f * far * near / (near - far), p.m[14]);
}

static void test_mat4_look_at(void) {
    vec3_t eye    = {0.0f, 0.0f, 5.0f, 0.0f};
    vec3_t center = {0.0f, 0.0f, 0.0f, 0.0f};
    vec3_t up     = {0.0f, 1.0f, 0.0f, 0.0f};
    mat4_t v = mc_math_mat4_look_at(eye, center, up);

    /* Transform the eye point itself: should produce (0,0,0,1) in view space */
    vec4_t eye_h = {0.0f, 0.0f, 5.0f, 1.0f};
    vec4_t r = mc_math_mat4_transform(v, eye_h);
    ASSERT_FLOAT_EQ("lookat eye.x", 0.0f, r.x);
    ASSERT_FLOAT_EQ("lookat eye.y", 0.0f, r.y);
    ASSERT_FLOAT_EQ("lookat eye.z", 0.0f, r.z);
}

static void test_mat4_multiply_translate(void) {
    /* Composing two translations should add offsets */
    mat4_t t1 = mc_math_mat4_translate((vec3_t){1.0f, 0.0f, 0.0f, 0.0f});
    mat4_t t2 = mc_math_mat4_translate((vec3_t){0.0f, 2.0f, 0.0f, 0.0f});
    mat4_t r = mc_math_mat4_multiply(t1, t2);
    ASSERT_FLOAT_EQ("mul_t[12]", 1.0f, r.m[12]);
    ASSERT_FLOAT_EQ("mul_t[13]", 2.0f, r.m[13]);
    ASSERT_FLOAT_EQ("mul_t[14]", 0.0f, r.m[14]);
}

static void test_quat_from_euler(void) {
    /* Zero angles should produce identity quaternion (0,0,0,1) */
    quat_t q = mc_math_quat_from_euler(0.0f, 0.0f, 0.0f);
    ASSERT_FLOAT_EQ("quat_id.x", 0.0f, q.x);
    ASSERT_FLOAT_EQ("quat_id.y", 0.0f, q.y);
    ASSERT_FLOAT_EQ("quat_id.z", 0.0f, q.z);
    ASSERT_FLOAT_EQ("quat_id.w", 1.0f, q.w);
}

static void test_quat_rotate(void) {
    /* 90-degree yaw (first param) rotates around Z: (1,0,0) -> (0,1,0) */
    float angle = (float)(M_PI / 2.0);
    quat_t q = mc_math_quat_from_euler(angle, 0.0f, 0.0f);
    vec3_t v = {1.0f, 0.0f, 0.0f, 0.0f};
    vec3_t r = mc_math_quat_rotate(q, v);
    ASSERT_FLOAT_EQ("quat_rot.x", 0.0f, r.x);
    ASSERT_FLOAT_EQ("quat_rot.y", 1.0f, r.y);
    ASSERT_FLOAT_EQ("quat_rot.z", 0.0f, r.z);
}

static void test_aabb_intersects(void) {
    aabb_t a = {{0,0,0,0}, {2,2,2,0}};
    aabb_t b = {{1,1,1,0}, {3,3,3,0}};
    aabb_t c = {{5,5,5,0}, {6,6,6,0}};
    ASSERT_FLOAT_EQ("aabb_intersects(overlap)", 1.0f, (float)mc_math_aabb_intersects(a, b));
    ASSERT_FLOAT_EQ("aabb_intersects(no overlap)", 0.0f, (float)mc_math_aabb_intersects(a, c));
}

static void test_aabb_contains(void) {
    aabb_t box = {{0,0,0,0}, {10,10,10,0}};
    vec3_t inside  = {5.0f, 5.0f, 5.0f, 0.0f};
    vec3_t outside = {15.0f, 5.0f, 5.0f, 0.0f};
    ASSERT_FLOAT_EQ("aabb_contains(in)", 1.0f, (float)mc_math_aabb_contains(box, inside));
    ASSERT_FLOAT_EQ("aabb_contains(out)", 0.0f, (float)mc_math_aabb_contains(box, outside));
}

static void test_noise_perlin2d(void) {
    /* Noise at integer coordinates should be 0 for classic Perlin */
    float n = mc_math_noise_perlin2d(0.0f, 0.0f, 42);
    ASSERT_FLOAT_EQ("perlin2d(0,0)", 0.0f, n);

    /* Non-integer coords should produce a non-trivial value in [-1,1] */
    float n2 = mc_math_noise_perlin2d(0.5f, 0.5f, 42);
    if (fabsf(n2) < 1e-6f) {
        g_fail++;
        fprintf(stderr, "FAIL perlin2d(0.5,0.5) is zero (degenerate)\n");
    } else if (n2 > 1.0f || n2 < -1.0f) {
        g_fail++;
        fprintf(stderr, "FAIL perlin2d(0.5,0.5) out of range: %.6f\n", n2);
    } else {
        g_pass++;
    }
}

static void test_noise_perlin3d(void) {
    float n = mc_math_noise_perlin3d(0.0f, 0.0f, 0.0f, 42);
    ASSERT_FLOAT_EQ("perlin3d(0,0,0)", 0.0f, n);

    float n2 = mc_math_noise_perlin3d(0.5f, 0.5f, 0.5f, 42);
    if (fabsf(n2) < 1e-6f) {
        g_fail++;
        fprintf(stderr, "FAIL perlin3d(0.5,0.5,0.5) is zero (degenerate)\n");
    } else if (n2 > 1.0f || n2 < -1.0f) {
        g_fail++;
        fprintf(stderr, "FAIL perlin3d(0.5,0.5,0.5) out of range: %.6f\n", n2);
    } else {
        g_pass++;
    }
}

static void test_noise_simplex2d(void) {
    float n = mc_math_noise_simplex2d(0.5f, 0.5f, 42);
    if (n > 1.0f || n < -1.0f) {
        g_fail++;
        fprintf(stderr, "FAIL simplex2d out of range: %.6f\n", n);
    } else {
        g_pass++;
    }

    /* Different seeds should give different results */
    float n1 = mc_math_noise_simplex2d(1.23f, 4.56f, 100);
    float n2 = mc_math_noise_simplex2d(1.23f, 4.56f, 200);
    if (fabsf(n1 - n2) < 1e-6f) {
        g_fail++;
        fprintf(stderr, "FAIL simplex2d same for different seeds\n");
    } else {
        g_pass++;
    }
}

int main(void) {
    test_mat4_identity();
    test_mat4_multiply_identity();
    test_mat4_translate();
    test_mat4_transform();
    test_mat4_rotate_x();
    test_mat4_rotate_y();
    test_mat4_perspective();
    test_mat4_look_at();
    test_mat4_multiply_translate();
    test_quat_from_euler();
    test_quat_rotate();
    test_aabb_intersects();
    test_aabb_contains();
    test_noise_perlin2d();
    test_noise_perlin3d();
    test_noise_simplex2d();

    printf("test_mat4: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
