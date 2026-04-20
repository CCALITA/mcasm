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

#define ASSERT_VEC3_EQ(msg, ex, ey, ez, v) do { \
    ASSERT_FLOAT_EQ(msg " .x", (ex), (v).x); \
    ASSERT_FLOAT_EQ(msg " .y", (ey), (v).y); \
    ASSERT_FLOAT_EQ(msg " .z", (ez), (v).z); \
} while (0)

static void test_vec3_add(void) {
    vec3_t a = {1.0f, 2.0f, 3.0f, 0.0f};
    vec3_t b = {4.0f, 5.0f, 6.0f, 0.0f};
    vec3_t r = mc_math_vec3_add(a, b);
    ASSERT_VEC3_EQ("vec3_add", 5.0f, 7.0f, 9.0f, r);
}

static void test_vec3_sub(void) {
    vec3_t a = {5.0f, 7.0f, 9.0f, 0.0f};
    vec3_t b = {1.0f, 2.0f, 3.0f, 0.0f};
    vec3_t r = mc_math_vec3_sub(a, b);
    ASSERT_VEC3_EQ("vec3_sub", 4.0f, 5.0f, 6.0f, r);
}

static void test_vec3_scale(void) {
    vec3_t v = {2.0f, 3.0f, 4.0f, 0.0f};
    vec3_t r = mc_math_vec3_scale(v, 2.5f);
    ASSERT_VEC3_EQ("vec3_scale", 5.0f, 7.5f, 10.0f, r);
}

static void test_vec3_dot(void) {
    vec3_t a = {1.0f, 2.0f, 3.0f, 0.0f};
    vec3_t b = {4.0f, 5.0f, 6.0f, 0.0f};
    float d = mc_math_vec3_dot(a, b);
    ASSERT_FLOAT_EQ("vec3_dot", 32.0f, d);
}

static void test_vec3_cross(void) {
    vec3_t a = {1.0f, 0.0f, 0.0f, 0.0f};
    vec3_t b = {0.0f, 1.0f, 0.0f, 0.0f};
    vec3_t r = mc_math_vec3_cross(a, b);
    ASSERT_VEC3_EQ("vec3_cross(x,y)", 0.0f, 0.0f, 1.0f, r);

    /* Non-trivial cross product */
    vec3_t c = {1.0f, 2.0f, 3.0f, 0.0f};
    vec3_t d = {4.0f, 5.0f, 6.0f, 0.0f};
    vec3_t r2 = mc_math_vec3_cross(c, d);
    ASSERT_VEC3_EQ("vec3_cross(arb)", -3.0f, 6.0f, -3.0f, r2);
}

static void test_vec3_length(void) {
    vec3_t v = {3.0f, 4.0f, 0.0f, 0.0f};
    float l = mc_math_vec3_length(v);
    ASSERT_FLOAT_EQ("vec3_length", 5.0f, l);
}

static void test_vec3_normalize(void) {
    vec3_t v = {3.0f, 0.0f, 4.0f, 0.0f};
    vec3_t r = mc_math_vec3_normalize(v);
    ASSERT_VEC3_EQ("vec3_normalize", 0.6f, 0.0f, 0.8f, r);

    /* Length of normalized vector should be 1 */
    float len = mc_math_vec3_length(r);
    ASSERT_FLOAT_EQ("vec3_normalize len", 1.0f, len);

    /* Zero vector stays zero */
    vec3_t z = {0.0f, 0.0f, 0.0f, 0.0f};
    vec3_t rz = mc_math_vec3_normalize(z);
    ASSERT_VEC3_EQ("vec3_normalize(0)", 0.0f, 0.0f, 0.0f, rz);
}

static void test_vec4_add(void) {
    vec4_t a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4_t b = {5.0f, 6.0f, 7.0f, 8.0f};
    vec4_t r = mc_math_vec4_add(a, b);
    ASSERT_FLOAT_EQ("vec4_add.x", 6.0f, r.x);
    ASSERT_FLOAT_EQ("vec4_add.y", 8.0f, r.y);
    ASSERT_FLOAT_EQ("vec4_add.z", 10.0f, r.z);
    ASSERT_FLOAT_EQ("vec4_add.w", 12.0f, r.w);
}

static void test_vec4_dot(void) {
    vec4_t a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4_t b = {5.0f, 6.0f, 7.0f, 8.0f};
    float d = mc_math_vec4_dot(a, b);
    ASSERT_FLOAT_EQ("vec4_dot", 70.0f, d);
}

int main(void) {
    test_vec3_add();
    test_vec3_sub();
    test_vec3_scale();
    test_vec3_dot();
    test_vec3_cross();
    test_vec3_length();
    test_vec3_normalize();
    test_vec4_add();
    test_vec4_dot();

    printf("test_vec3: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
