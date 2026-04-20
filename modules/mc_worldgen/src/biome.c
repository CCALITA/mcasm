/*
 * biome.c -- Biome assignment based on temperature, humidity, and
 *            continentalness noise.
 *
 * Temperature and humidity are each sampled from independent noise
 * with different seed offsets, then mapped to a biome via a lookup
 * grid.  A separate "continentalness" noise field decides whether
 * terrain is ocean or land; when the value is below a threshold the
 * column is ocean regardless of temperature/humidity.
 */

#include "worldgen_internal.h"
#include "mc_worldgen.h"

#define TEMP_OFFSET_X  5000.0f
#define TEMP_OFFSET_Z  3000.0f
#define HUM_OFFSET_X   9000.0f
#define HUM_OFFSET_Z   7000.0f
#define CONT_OFFSET_X  4000.0f
#define CONT_OFFSET_Z  8000.0f
#define BIOME_SCALE    256.0f
#define CONT_SCALE     200.0f
#define OCEAN_THRESHOLD (-0.3f)

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

/* Returns continentalness in [-1, 1].  Low values = ocean. */
static float continentalness(int32_t x, int32_t z)
{
    float nx = ((float)x + CONT_OFFSET_X) / CONT_SCALE;
    float nz = ((float)z + CONT_OFFSET_Z) / CONT_SCALE;
    return noise_octave2d(nx, nz, 3, 0.5f, 2.0f);
}

/*
 * Biome selection via continentalness + temperature / humidity.
 *
 * If continentalness < OCEAN_THRESHOLD  ->  OCEAN
 *
 * Otherwise, temperature / humidity grid:
 *
 *              Humidity
 *         Low          Mid          High
 * Cold    TUNDRA       TAIGA        TAIGA
 * Mid     PLAINS       FOREST       SWAMP
 * Hot     DESERT       SAVANNA      JUNGLE
 *
 * MOUNTAINS override: when continentalness > 0.6 and temp is mid.
 */
uint8_t biome_get(int32_t x, int32_t z)
{
    float cont = continentalness(x, z);

    if (cont < OCEAN_THRESHOLD) {
        return BIOME_OCEAN;
    }

    float temp = biome_temperature(x, z);
    float hum  = biome_humidity(x, z);

    /* High continentalness in temperate band = mountains */
    if (cont > 0.6f && temp >= 0.25f && temp < 0.55f) {
        return BIOME_MOUNTAINS;
    }

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
