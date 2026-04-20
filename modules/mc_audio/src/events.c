#include "mc_audio.h"
#include "audio_internal.h"
#include "audio_events.h"

#include <math.h>
#include <stdlib.h>

/* ------- constants ------- */

#define SAMPLE_RATE    22050
#define BITS           16
#define CHANNELS       1

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ------- PCM generators ------- */

typedef enum {
    TONE_SINE,
    TONE_NOISE
} tone_type_t;

typedef struct {
    tone_type_t type;
    uint32_t    freq_hz;      /* ignored for noise */
    uint32_t    duration_ms;
} tone_desc_t;

/*
 * Sound-design table: maps each SOUND_* event to its placeholder tone.
 */
static const tone_desc_t g_event_tones[SOUND_EVENT_COUNT] = {
    [SOUND_FOOTSTEP]      = { TONE_SINE,  200,  50 },  /* low click   */
    [SOUND_BLOCK_BREAK]   = { TONE_NOISE,   0,  80 },  /* crunch      */
    [SOUND_BLOCK_PLACE]   = { TONE_SINE,  150,  60 },  /* thud        */
    [SOUND_HURT]          = { TONE_SINE,  800, 100 },  /* sharp tone  */
    [SOUND_AMBIENT_CAVE]  = { TONE_SINE,   80, 200 },  /* low rumble  */
    [SOUND_WATER_AMBIENT] = { TONE_SINE,  300, 150 },  /* burble      */
    [SOUND_MUSIC]         = { TONE_SINE,  440, 500 },  /* placeholder */
};

/*
 * Generate a 16-bit mono PCM buffer.
 * Returns malloc'd buffer (caller frees).  *out_size receives byte count.
 */
static int16_t *gen_tone(const tone_desc_t *desc, uint32_t *out_size)
{
    uint32_t num_samples = (SAMPLE_RATE * desc->duration_ms) / 1000;
    uint32_t byte_count  = num_samples * sizeof(int16_t);

    int16_t *buf = (int16_t *)malloc(byte_count);
    if (!buf) {
        return NULL;
    }

    if (desc->type == TONE_NOISE) {
        /* Deterministic LCG white noise */
        uint32_t seed = 0xDEADBEEF;
        for (uint32_t i = 0; i < num_samples; i++) {
            seed = seed * 1103515245 + 12345;
            buf[i] = (int16_t)((int16_t)((seed >> 16) & 0xFFFF) / 2);
        }
    } else {
        /* Sine wave */
        for (uint32_t i = 0; i < num_samples; i++) {
            double t = (double)i / (double)SAMPLE_RATE;
            buf[i] = (int16_t)(sin(2.0 * M_PI * (double)desc->freq_hz * t) * 32000.0);
        }
    }

    *out_size = byte_count;
    return buf;
}

/* ------- state ------- */

static uint32_t g_event_sounds[SOUND_EVENT_COUNT];
static int      g_events_initialized;

/* ------- public API ------- */

void mc_audio_events_init(void)
{
    if (g_events_initialized) {
        return;
    }

    for (uint8_t i = 0; i < SOUND_EVENT_COUNT; i++) {
        uint32_t sz;
        int16_t *pcm = gen_tone(&g_event_tones[i], &sz);
        if (pcm) {
            g_event_sounds[i] =
                mc_audio_load_sound_pcm(pcm, sz, CHANNELS, BITS, SAMPLE_RATE);
            free(pcm);
        }
    }

    g_events_initialized = 1;
}

void mc_audio_play_event(uint8_t event, vec3_t position)
{
    if (!g_events_initialized) {
        return;
    }
    if (event >= SOUND_EVENT_COUNT) {
        return;
    }

    uint32_t handle = g_event_sounds[event];
    if (handle == 0) {
        return;
    }

    mc_audio_play(handle, position, 1.0f, 1.0f);
}
