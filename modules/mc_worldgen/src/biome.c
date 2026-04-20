/*
 * biome.c -- Biome assignment based on temperature and humidity noise.
 *
 * Temperature and humidity are each sampled from independent noise
 * with different seed offsets, then mapped to a biome via a lookup
 * grid.
 */

#include "worldgen_internal.h"
#include "mc_worldgen.h"

#define TEMP_OFFSET_X  5000.0f
#define TEMP_OFFSET_Z  3000.0f
#define HUM_OFFSET_X   9000.0f
#define HUM_OFFSET_Z   7000.0f
#define BIOME_SCALE    256.0f

void biome_init(uint32_t seed)
{
    (void)seed;
}

/* Returns temperature in [0, 1]. */
float biome_temperature(int32_t x, int32_t z)
{
    float nx = ((float)x + TEMP_OFFSET_X) / BIOME_SCALE;
    float nz = ((float)z + TEMP_OFFSET_Z) / BIOME_SCALE;
    float t = noise_octave2d(nx, nz, 4, 0.5f, 2.0f);
    /* Map [-1,1] -> [0,1] */
    return (t + 1.0f) * 0.5f;
}

/* Returns humidity in [0, 1]. */
float biome_humidity(int32_t x, int32_t z)
{
    float nx = ((float)x + HUM_OFFSET_X) / BIOME_SCALE;
    float nz = ((float)z + HUM_OFFSET_Z) / BIOME_SCALE;
    float h = noise_octave2d(nx, nz, 4, 0.5f, 2.0f);
    return (h + 1.0f) * 0.5f;
}

/*
 * Biome selection via temperature / humidity thresholds.
 *
 *              Humidity
 *         Low        Mid        High
 * Cold    TUNDRA     TAIGA      TAIGA
 * Mid     PLAINS     FOREST     SWAMP
 * Hot     DESERT     SAVANNA    JUNGLE
 */
uint8_t biome_get(int32_t x, int32_t z)
{
    float temp = biome_temperature(x, z);
    float hum  = biome_humidity(x, z);

    if (temp < 0.25f) {
        /* Cold */
        if (hum < 0.4f) return BIOME_TUNDRA;
        return BIOME_TAIGA;
    }

    if (temp < 0.55f) {
        /* Temperate */
        if (hum < 0.3f) return BIOME_PLAINS;
        if (hum < 0.65f) return BIOME_FOREST;
        return BIOME_SWAMP;
    }

    /* Hot */
    if (hum < 0.3f) return BIOME_DESERT;
    if (hum < 0.6f) return BIOME_SAVANNA;
    return BIOME_JUNGLE;
}
