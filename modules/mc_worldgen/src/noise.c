/*
 * noise.c -- Perlin noise implementation with permutation table.
 *
 * Provides 2D and 3D Perlin noise plus multi-octave (fractal) wrappers.
 * Values are in [-1, 1].  The permutation table is seeded once via
 * noise_init() so that different worlds produce different terrain.
 */

#include "worldgen_internal.h"
#include <math.h>

/* ------------------------------------------------------------------ */
/* Permutation table (doubled to avoid wrapping)                      */
/* ------------------------------------------------------------------ */

static uint8_t perm[512];

static void shuffle_perm(uint32_t seed)
{
    /* Fill 0..255, then Fisher-Yates shuffle seeded by a simple LCG. */
    for (int i = 0; i < 256; i++) {
        perm[i] = (uint8_t)i;
    }

    uint32_t s = seed;
    for (int i = 255; i > 0; i--) {
        s = s * 1103515245u + 12345u;
        int j = (int)((s >> 16) % (uint32_t)(i + 1));
        uint8_t tmp = perm[i];
        perm[i] = perm[j];
        perm[j] = tmp;
    }

    /* Duplicate so we never need to mask. */
    for (int i = 0; i < 256; i++) {
        perm[256 + i] = perm[i];
    }
}

void noise_init(uint32_t seed)
{
    shuffle_perm(seed);
}

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

static inline float fade(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static inline float lerp_f(float a, float b, float t)
{
    return a + t * (b - a);
}

static inline float grad2d(int hash, float x, float y)
{
    int h = hash & 3;
    float u = (h & 2) ? -x : x;
    float v = (h & 1) ? -y : y;
    return u + v;
}

static inline float grad3d(int hash, float x, float y, float z)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

/* ------------------------------------------------------------------ */
/* 2D Perlin noise                                                    */
/* ------------------------------------------------------------------ */

float noise_perlin2d(float x, float y)
{
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;

    float xf = x - floorf(x);
    float yf = y - floorf(y);

    float u = fade(xf);
    float v = fade(yf);

    int aa = perm[perm[xi]     + yi];
    int ab = perm[perm[xi]     + yi + 1];
    int ba = perm[perm[xi + 1] + yi];
    int bb = perm[perm[xi + 1] + yi + 1];

    float x1 = lerp_f(grad2d(aa, xf,        yf),
                       grad2d(ba, xf - 1.0f, yf),        u);
    float x2 = lerp_f(grad2d(ab, xf,        yf - 1.0f),
                       grad2d(bb, xf - 1.0f, yf - 1.0f), u);

    return lerp_f(x1, x2, v);
}

/* ------------------------------------------------------------------ */
/* 3D Perlin noise                                                    */
/* ------------------------------------------------------------------ */

float noise_perlin3d(float x, float y, float z)
{
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    int zi = (int)floorf(z) & 255;

    float xf = x - floorf(x);
    float yf = y - floorf(y);
    float zf = z - floorf(z);

    float u = fade(xf);
    float v = fade(yf);
    float w = fade(zf);

    int a   = perm[xi]     + yi;
    int aa  = perm[a]      + zi;
    int ab  = perm[a + 1]  + zi;
    int b   = perm[xi + 1] + yi;
    int ba  = perm[b]      + zi;
    int bb  = perm[b + 1]  + zi;

    float x1 = lerp_f(grad3d(perm[aa],     xf,        yf,        zf),
                       grad3d(perm[ba],     xf - 1.0f, yf,        zf),        u);
    float x2 = lerp_f(grad3d(perm[ab],     xf,        yf - 1.0f, zf),
                       grad3d(perm[bb],     xf - 1.0f, yf - 1.0f, zf),        u);
    float y1 = lerp_f(x1, x2, v);

    float x3 = lerp_f(grad3d(perm[aa + 1], xf,        yf,        zf - 1.0f),
                       grad3d(perm[ba + 1], xf - 1.0f, yf,        zf - 1.0f), u);
    float x4 = lerp_f(grad3d(perm[ab + 1], xf,        yf - 1.0f, zf - 1.0f),
                       grad3d(perm[bb + 1], xf - 1.0f, yf - 1.0f, zf - 1.0f), u);
    float y2 = lerp_f(x3, x4, v);

    return lerp_f(y1, y2, w);
}

/* ------------------------------------------------------------------ */
/* Multi-octave wrappers                                              */
/* ------------------------------------------------------------------ */

float noise_octave2d(float x, float y, int octaves, float persistence, float lacunarity)
{
    float total     = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float max_val   = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total   += noise_perlin2d(x * frequency, y * frequency) * amplitude;
        max_val += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return total / max_val;
}

float noise_octave3d(float x, float y, float z, int octaves, float persistence, float lacunarity)
{
    float total     = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float max_val   = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total   += noise_perlin3d(x * frequency, y * frequency, z * frequency) * amplitude;
        max_val += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return total / max_val;
}
