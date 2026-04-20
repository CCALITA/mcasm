#include "mc_math.h"
#include <math.h>
#include <string.h>

/* ---- Permutation table (classic Perlin approach, seeded) ---- */

static void build_perm(uint8_t perm[512], uint32_t seed) {
    uint8_t base[256];
    for (int i = 0; i < 256; i++) {
        base[i] = (uint8_t)i;
    }
    /* Fisher-Yates shuffle seeded with a simple LCG */
    uint32_t s = seed;
    for (int i = 255; i > 0; i--) {
        s = s * 1664525u + 1013904223u;
        int j = (int)((s >> 16) % (uint32_t)(i + 1));
        uint8_t tmp = base[i];
        base[i] = base[j];
        base[j] = tmp;
    }
    memcpy(perm, base, 256);
    memcpy(perm + 256, base, 256);
}

/* ---- Gradient helpers ---- */

static float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float lerp_f(float t, float a, float b) {
    return a + t * (b - a);
}

static float grad2d(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

static float grad3d(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

/* ---- Perlin 2D ---- */

float mc_math_noise_perlin2d(float x, float y, uint32_t seed) {
    uint8_t perm[512];
    build_perm(perm, seed);

    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    float xf = x - floorf(x);
    float yf = y - floorf(y);
    float u = fade(xf);
    float v = fade(yf);

    int aa = perm[perm[xi] + yi];
    int ab = perm[perm[xi] + yi + 1];
    int ba = perm[perm[xi + 1] + yi];
    int bb = perm[perm[xi + 1] + yi + 1];

    float x1 = lerp_f(u, grad2d(aa, xf, yf), grad2d(ba, xf - 1.0f, yf));
    float x2 = lerp_f(u, grad2d(ab, xf, yf - 1.0f), grad2d(bb, xf - 1.0f, yf - 1.0f));
    return lerp_f(v, x1, x2);
}

/* ---- Perlin 3D ---- */

float mc_math_noise_perlin3d(float x, float y, float z, uint32_t seed) {
    uint8_t perm[512];
    build_perm(perm, seed);

    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    int zi = (int)floorf(z) & 255;
    float xf = x - floorf(x);
    float yf = y - floorf(y);
    float zf = z - floorf(z);
    float u = fade(xf);
    float v = fade(yf);
    float w = fade(zf);

    int a  = perm[xi] + yi;
    int aa = perm[a] + zi;
    int ab = perm[a + 1] + zi;
    int b  = perm[xi + 1] + yi;
    int ba = perm[b] + zi;
    int bb = perm[b + 1] + zi;

    float x1 = lerp_f(u, grad3d(perm[aa],     xf,       yf,       zf),
                          grad3d(perm[ba],     xf-1.0f,  yf,       zf));
    float x2 = lerp_f(u, grad3d(perm[ab],     xf,       yf-1.0f,  zf),
                          grad3d(perm[bb],     xf-1.0f,  yf-1.0f,  zf));
    float y1 = lerp_f(v, x1, x2);

    float x3 = lerp_f(u, grad3d(perm[aa+1],   xf,       yf,       zf-1.0f),
                          grad3d(perm[ba+1],   xf-1.0f,  yf,       zf-1.0f));
    float x4 = lerp_f(u, grad3d(perm[ab+1],   xf,       yf-1.0f,  zf-1.0f),
                          grad3d(perm[bb+1],   xf-1.0f,  yf-1.0f,  zf-1.0f));
    float y2 = lerp_f(v, x3, x4);

    return lerp_f(w, y1, y2);
}

/* ---- Simplex 2D ---- */

static const float F2 = 0.3660254037844386f;  /* (sqrt(3) - 1) / 2 */
static const float G2 = 0.2113248654051871f;  /* (3 - sqrt(3)) / 6 */

static float grad2d_simplex(int hash, float x, float y) {
    static const float grad[][2] = {
        { 1, 1}, {-1, 1}, { 1,-1}, {-1,-1},
        { 1, 0}, {-1, 0}, { 0, 1}, { 0,-1},
    };
    int h = hash & 7;
    return grad[h][0] * x + grad[h][1] * y;
}

float mc_math_noise_simplex2d(float x, float y, uint32_t seed) {
    uint8_t perm[512];
    build_perm(perm, seed);

    float s = (x + y) * F2;
    int i = (int)floorf(x + s);
    int j = (int)floorf(y + s);
    float t = (float)(i + j) * G2;
    float x0 = x - ((float)i - t);
    float y0 = y - ((float)j - t);

    int i1, j1;
    if (x0 > y0) {
        i1 = 1; j1 = 0;
    } else {
        i1 = 0; j1 = 1;
    }

    float x1 = x0 - (float)i1 + G2;
    float y1 = y0 - (float)j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;

    int ii = i & 255;
    int jj = j & 255;
    int gi0 = perm[ii + perm[jj]];
    int gi1 = perm[ii + i1 + perm[jj + j1]];
    int gi2 = perm[ii + 1 + perm[jj + 1]];

    float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f;

    float t0 = 0.5f - x0 * x0 - y0 * y0;
    if (t0 >= 0.0f) {
        t0 *= t0;
        n0 = t0 * t0 * grad2d_simplex(gi0, x0, y0);
    }

    float t1 = 0.5f - x1 * x1 - y1 * y1;
    if (t1 >= 0.0f) {
        t1 *= t1;
        n1 = t1 * t1 * grad2d_simplex(gi1, x1, y1);
    }

    float t2 = 0.5f - x2 * x2 - y2 * y2;
    if (t2 >= 0.0f) {
        t2 *= t2;
        n2 = t2 * t2 * grad2d_simplex(gi2, x2, y2);
    }

    /* Scale to [-1, 1] range (70 is the standard simplex 2D scaling factor) */
    return 70.0f * (n0 + n1 + n2);
}
