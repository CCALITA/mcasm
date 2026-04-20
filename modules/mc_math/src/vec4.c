#include "mc_math.h"

#ifdef __SSE4_1__
#include <xmmintrin.h>
#include <smmintrin.h>
#endif

vec4_t mc_math_vec4_add(vec4_t a, vec4_t b) {
#ifdef __SSE4_1__
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 vr = _mm_add_ps(va, vb);
    vec4_t r;
    _mm_store_ps(&r.x, vr);
    return r;
#else
    return (vec4_t){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
#endif
}

float mc_math_vec4_dot(vec4_t a, vec4_t b) {
#ifdef __SSE4_1__
    __m128 va = _mm_load_ps(&a.x);
    __m128 vb = _mm_load_ps(&b.x);
    __m128 dp = _mm_dp_ps(va, vb, 0xF1);
    return _mm_cvtss_f32(dp);
#else
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
#endif
}
