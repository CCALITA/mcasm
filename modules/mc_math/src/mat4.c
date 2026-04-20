#include "mc_math.h"
#include <math.h>
#include <string.h>

#ifdef __SSE4_1__
#include <xmmintrin.h>
#include <smmintrin.h>
#endif

mat4_t mc_math_mat4_identity(void) {
    mat4_t m;
    memset(&m, 0, sizeof(m));
    m.m[0] = 1.0f;
    m.m[5] = 1.0f;
    m.m[10] = 1.0f;
    m.m[15] = 1.0f;
    return m;
}

/*
 * Column-major 4x4 matrix multiply.
 * On SSE4.1, each result column is computed as a linear combination
 * of the four columns of A, weighted by the corresponding column entry of B.
 * On other platforms, falls back to a scalar triple loop.
 */
mat4_t mc_math_mat4_multiply(mat4_t a, mat4_t b) {
    mat4_t r;
#ifdef __SSE4_1__
    __m128 a_col0 = _mm_load_ps(&a.m[0]);
    __m128 a_col1 = _mm_load_ps(&a.m[4]);
    __m128 a_col2 = _mm_load_ps(&a.m[8]);
    __m128 a_col3 = _mm_load_ps(&a.m[12]);

    for (int col = 0; col < 4; col++) {
        __m128 b0 = _mm_set1_ps(b.m[col * 4 + 0]);
        __m128 b1 = _mm_set1_ps(b.m[col * 4 + 1]);
        __m128 b2 = _mm_set1_ps(b.m[col * 4 + 2]);
        __m128 b3 = _mm_set1_ps(b.m[col * 4 + 3]);
        __m128 result = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(a_col0, b0), _mm_mul_ps(a_col1, b1)),
            _mm_add_ps(_mm_mul_ps(a_col2, b2), _mm_mul_ps(a_col3, b3))
        );
        _mm_store_ps(&r.m[col * 4], result);
    }
#else
    memset(&r, 0, sizeof(r));
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            for (int k = 0; k < 4; k++) {
                r.m[col * 4 + row] += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
        }
    }
#endif
    return r;
}

mat4_t mc_math_mat4_translate(vec3_t offset) {
    mat4_t m = mc_math_mat4_identity();
    m.m[12] = offset.x;
    m.m[13] = offset.y;
    m.m[14] = offset.z;
    return m;
}

mat4_t mc_math_mat4_rotate_x(float radians) {
    float c = cosf(radians);
    float s = sinf(radians);
    mat4_t m = mc_math_mat4_identity();
    m.m[5] = c;
    m.m[6] = s;
    m.m[9] = -s;
    m.m[10] = c;
    return m;
}

mat4_t mc_math_mat4_rotate_y(float radians) {
    float c = cosf(radians);
    float s = sinf(radians);
    mat4_t m = mc_math_mat4_identity();
    m.m[0] = c;
    m.m[2] = -s;
    m.m[8] = s;
    m.m[10] = c;
    return m;
}

mat4_t mc_math_mat4_perspective(float fov, float aspect, float near, float far) {
    mat4_t m;
    memset(&m, 0, sizeof(m));
    float f = 1.0f / tanf(fov / 2.0f);
    m.m[0] = f / aspect;
    m.m[5] = f;
    m.m[10] = (far + near) / (near - far);
    m.m[11] = -1.0f;
    m.m[14] = 2.0f * far * near / (near - far);
    return m;
}

mat4_t mc_math_mat4_look_at(vec3_t eye, vec3_t center, vec3_t up) {
    vec3_t f = mc_math_vec3_normalize(mc_math_vec3_sub(center, eye));
    vec3_t s = mc_math_vec3_normalize(mc_math_vec3_cross(f, up));
    vec3_t u = mc_math_vec3_cross(s, f);

    mat4_t m = mc_math_mat4_identity();
    m.m[0] = s.x;   m.m[4] = s.y;   m.m[8]  = s.z;
    m.m[1] = u.x;   m.m[5] = u.y;   m.m[9]  = u.z;
    m.m[2] = -f.x;  m.m[6] = -f.y;  m.m[10] = -f.z;
    m.m[12] = -mc_math_vec3_dot(s, eye);
    m.m[13] = -mc_math_vec3_dot(u, eye);
    m.m[14] = mc_math_vec3_dot(f, eye);
    return m;
}

vec4_t mc_math_mat4_transform(mat4_t m, vec4_t v) {
#ifdef __SSE4_1__
    __m128 vv = _mm_load_ps(&v.x);
    __m128 col0 = _mm_load_ps(&m.m[0]);
    __m128 col1 = _mm_load_ps(&m.m[4]);
    __m128 col2 = _mm_load_ps(&m.m[8]);
    __m128 col3 = _mm_load_ps(&m.m[12]);
    __m128 result = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(col0, _mm_shuffle_ps(vv, vv, _MM_SHUFFLE(0, 0, 0, 0))),
            _mm_mul_ps(col1, _mm_shuffle_ps(vv, vv, _MM_SHUFFLE(1, 1, 1, 1)))
        ),
        _mm_add_ps(
            _mm_mul_ps(col2, _mm_shuffle_ps(vv, vv, _MM_SHUFFLE(2, 2, 2, 2))),
            _mm_mul_ps(col3, _mm_shuffle_ps(vv, vv, _MM_SHUFFLE(3, 3, 3, 3)))
        )
    );
    vec4_t r;
    _mm_store_ps(&r.x, result);
    return r;
#else
    return (vec4_t){
        m.m[0]*v.x + m.m[4]*v.y + m.m[8]*v.z  + m.m[12]*v.w,
        m.m[1]*v.x + m.m[5]*v.y + m.m[9]*v.z  + m.m[13]*v.w,
        m.m[2]*v.x + m.m[6]*v.y + m.m[10]*v.z + m.m[14]*v.w,
        m.m[3]*v.x + m.m[7]*v.y + m.m[11]*v.z + m.m[15]*v.w
    };
#endif
}
