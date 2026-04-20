#include "mc_math.h"
#include <math.h>

#ifdef __SSE4_1__
#include <xmmintrin.h>
#include <smmintrin.h>
#endif

vec3_t mc_math_vec3_add(vec3_t a, vec3_t b) {
#ifdef __SSE4_1__
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 vr = _mm_add_ps(va, vb);
    vec3_t r;
    _mm_store_ps(&r.x, vr);
    r._pad = 0.0f;
    return r;
#else
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z, 0.0f};
#endif
}

vec3_t mc_math_vec3_sub(vec3_t a, vec3_t b) {
#ifdef __SSE4_1__
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 vr = _mm_sub_ps(va, vb);
    vec3_t r;
    _mm_store_ps(&r.x, vr);
    r._pad = 0.0f;
    return r;
#else
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z, 0.0f};
#endif
}

vec3_t mc_math_vec3_scale(vec3_t v, float s) {
#ifdef __SSE4_1__
    __m128 vv = _mm_load_ps(&v.x);
    __m128 vs = _mm_set1_ps(s);
    __m128 vr = _mm_mul_ps(vv, vs);
    vec3_t r;
    _mm_store_ps(&r.x, vr);
    r._pad = 0.0f;
    return r;
#else
    return (vec3_t){v.x * s, v.y * s, v.z * s, 0.0f};
#endif
}

float mc_math_vec3_dot(vec3_t a, vec3_t b) {
#ifdef __SSE4_1__
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 dp = _mm_dp_ps(va, vb, 0x71);
    return _mm_cvtss_f32(dp);
#else
    return a.x * b.x + a.y * b.y + a.z * b.z;
#endif
}

vec3_t mc_math_vec3_cross(vec3_t a, vec3_t b) {
#ifdef __SSE4_1__
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 a_yzx = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 b_zxy = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 a_zxy = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 b_yzx = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 vr = _mm_sub_ps(_mm_mul_ps(a_yzx, b_zxy), _mm_mul_ps(a_zxy, b_yzx));
    vec3_t r;
    _mm_store_ps(&r.x, vr);
    r._pad = 0.0f;
    return r;
#else
    return (vec3_t){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
        0.0f
    };
#endif
}

float mc_math_vec3_length(vec3_t v) {
    return sqrtf(mc_math_vec3_dot(v, v));
}

vec3_t mc_math_vec3_normalize(vec3_t v) {
    float len = mc_math_vec3_length(v);
    if (len > 0.0f) {
        return mc_math_vec3_scale(v, 1.0f / len);
    }
    return v;
}
