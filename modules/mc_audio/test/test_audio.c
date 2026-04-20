#include "mc_audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../internal/audio_internal.h"

/* ------- test helpers ------- */

static int g_tests_run    = 0;
static int g_tests_passed = 0;

#define TEST(name)  static void name(void)
#define RUN(name)   do { \
    g_tests_run++; \
    printf("  %-50s", #name); \
    name(); \
    g_tests_passed++; \
    printf("PASS\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n    assert failed: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)

/* ------- synthetic WAV builder ------- */

/*
 * Build a minimal valid WAV file in memory.
 * Returns malloc'd buffer (caller frees). Sets *out_size.
 */
static void *make_wav(uint16_t channels, uint16_t bits, uint32_t sample_rate,
                      uint32_t num_samples, size_t *out_size)
{
    uint16_t block_align = (uint16_t)(channels * (bits / 8));
    uint32_t byte_rate   = sample_rate * block_align;
    uint32_t data_size   = num_samples * block_align;
    uint32_t file_size   = 44 + data_size;

    uint8_t *buf = (uint8_t *)calloc(1, file_size);
    if (!buf) return NULL;
    uint8_t *p = buf;

    /* RIFF header */
    memcpy(p, "RIFF", 4); p += 4;
    uint32_t riff_sz = file_size - 8;
    memcpy(p, &riff_sz, 4); p += 4;
    memcpy(p, "WAVE", 4); p += 4;

    /* fmt chunk */
    memcpy(p, "fmt ", 4); p += 4;
    uint32_t fmt_sz = 16;
    memcpy(p, &fmt_sz, 4); p += 4;
    uint16_t audio_fmt = 1; /* PCM */
    memcpy(p, &audio_fmt, 2); p += 2;
    memcpy(p, &channels, 2); p += 2;
    memcpy(p, &sample_rate, 4); p += 4;
    memcpy(p, &byte_rate, 4); p += 4;
    memcpy(p, &block_align, 2); p += 2;
    memcpy(p, &bits, 2); p += 2;

    /* data chunk */
    memcpy(p, "data", 4); p += 4;
    memcpy(p, &data_size, 4); p += 4;

    /* Fill with simple ramp to verify data is copied */
    for (uint32_t i = 0; i < data_size; i++) {
        p[i] = (uint8_t)(i & 0xFF);
    }

    *out_size = file_size;
    return buf;
}

/* ------- WAV parser tests ------- */

TEST(test_wav_parse_mono8)
{
    size_t sz;
    void *buf = make_wav(1, 8, 22050, 100, &sz);
    ASSERT(buf != NULL);

    wav_data_t wav;
    mc_error_t err = wav_parse(buf, sz, &wav);
    ASSERT(err == MC_OK);
    ASSERT(wav.channels == 1);
    ASSERT(wav.bits_per_sample == 8);
    ASSERT(wav.sample_rate == 22050);
    ASSERT(wav.data_size == 100);
    ASSERT(wav.data != NULL);

    /* Verify PCM data was copied correctly (first byte) */
    ASSERT(((uint8_t *)wav.data)[0] == 0x00);

    free(wav.data);
    free(buf);
}

TEST(test_wav_parse_stereo16)
{
    size_t sz;
    void *buf = make_wav(2, 16, 44100, 50, &sz);
    ASSERT(buf != NULL);

    wav_data_t wav;
    mc_error_t err = wav_parse(buf, sz, &wav);
    ASSERT(err == MC_OK);
    ASSERT(wav.channels == 2);
    ASSERT(wav.bits_per_sample == 16);
    ASSERT(wav.sample_rate == 44100);
    /* 50 samples * 2 channels * 2 bytes = 200 */
    ASSERT(wav.data_size == 200);

    free(wav.data);
    free(buf);
}

TEST(test_wav_parse_mono16)
{
    size_t sz;
    void *buf = make_wav(1, 16, 48000, 200, &sz);
    ASSERT(buf != NULL);

    wav_data_t wav;
    mc_error_t err = wav_parse(buf, sz, &wav);
    ASSERT(err == MC_OK);
    ASSERT(wav.channels == 1);
    ASSERT(wav.bits_per_sample == 16);
    ASSERT(wav.sample_rate == 48000);
    ASSERT(wav.data_size == 400); /* 200 samples * 2 bytes */

    free(wav.data);
    free(buf);
}

TEST(test_wav_parse_null_args)
{
    wav_data_t wav;
    ASSERT(wav_parse(NULL, 0, &wav) == MC_ERR_INVALID_ARG);
    ASSERT(wav_parse("x", 1, NULL) == MC_ERR_INVALID_ARG);
}

TEST(test_wav_parse_truncated)
{
    uint8_t short_buf[10] = {0};
    wav_data_t wav;
    ASSERT(wav_parse(short_buf, sizeof(short_buf), &wav) == MC_ERR_IO);
}

TEST(test_wav_parse_bad_magic)
{
    size_t sz;
    void *buf = make_wav(1, 8, 22050, 10, &sz);
    ASSERT(buf != NULL);

    /* Corrupt RIFF magic */
    memcpy(buf, "XXXX", 4);
    wav_data_t wav;
    ASSERT(wav_parse(buf, sz, &wav) == MC_ERR_IO);

    free(buf);
}

TEST(test_wav_parse_bad_wave)
{
    size_t sz;
    void *buf = make_wav(1, 8, 22050, 10, &sz);
    ASSERT(buf != NULL);

    /* Corrupt WAVE magic */
    memcpy((uint8_t *)buf + 8, "XXXX", 4);
    wav_data_t wav;
    ASSERT(wav_parse(buf, sz, &wav) == MC_ERR_IO);

    free(buf);
}

/* ------- Audio init/shutdown tests ------- */

TEST(test_init_shutdown)
{
    mc_error_t err = mc_audio_init();
    /*
     * If OpenAL is available, init succeeds.
     * If running stub mode, init also returns MC_OK.
     */
    ASSERT(err == MC_OK);
    mc_audio_shutdown();
}

TEST(test_double_init)
{
    mc_error_t err1 = mc_audio_init();
    ASSERT(err1 == MC_OK);
    mc_error_t err2 = mc_audio_init();
    ASSERT(err2 == MC_OK);
    mc_audio_shutdown();
}

TEST(test_shutdown_without_init)
{
    /* Should not crash */
    mc_audio_shutdown();
}

TEST(test_set_master_volume)
{
    mc_error_t err = mc_audio_init();
    ASSERT(err == MC_OK);

    /* Should not crash with various values */
    mc_audio_set_master_volume(0.5f);
    mc_audio_set_master_volume(0.0f);
    mc_audio_set_master_volume(1.0f);
    mc_audio_set_master_volume(-1.0f); /* clamped */
    mc_audio_set_master_volume(2.0f);  /* clamped */

    mc_audio_shutdown();
}

TEST(test_set_listener)
{
    mc_error_t err = mc_audio_init();
    ASSERT(err == MC_OK);

    vec3_t pos = { 1.0f, 2.0f, 3.0f, 0.0f };
    vec3_t fwd = { 0.0f, 0.0f, -1.0f, 0.0f };
    vec3_t up  = { 0.0f, 1.0f, 0.0f, 0.0f };
    mc_audio_set_listener(pos, fwd, up);

    mc_audio_shutdown();
}

TEST(test_tick_no_crash)
{
    mc_error_t err = mc_audio_init();
    ASSERT(err == MC_OK);
    mc_audio_tick();
    mc_audio_tick();
    mc_audio_shutdown();
}

TEST(test_stop_music_no_crash)
{
    mc_error_t err = mc_audio_init();
    ASSERT(err == MC_OK);
    /* Stopping music when none is playing should be fine */
    mc_audio_stop_music();
    mc_audio_shutdown();
}

TEST(test_play_invalid_sound)
{
    mc_error_t err = mc_audio_init();
    ASSERT(err == MC_OK);

    vec3_t pos = { 0.0f, 0.0f, 0.0f, 0.0f };
    /* Sound 0 is always invalid, sound 999 is out of range */
    mc_audio_play(0, pos, 1.0f, 1.0f);
    mc_audio_play(999, pos, 1.0f, 1.0f);

    mc_audio_shutdown();
}

TEST(test_operations_without_init)
{
    vec3_t pos = { 0.0f, 0.0f, 0.0f, 0.0f };
    vec3_t fwd = { 0.0f, 0.0f, -1.0f, 0.0f };
    vec3_t up  = { 0.0f, 1.0f, 0.0f, 0.0f };

    /* None of these should crash when not initialized */
    mc_audio_play(1, pos, 1.0f, 1.0f);
    mc_audio_play_music("nonexistent.wav", 1.0f);
    mc_audio_stop_music();
    mc_audio_set_listener(pos, fwd, up);
    mc_audio_set_master_volume(0.5f);
    mc_audio_tick();
    ASSERT(mc_audio_load_sound("nonexistent.wav") == 0);
}

/* ------- main ------- */

int main(void)
{
    printf("mc_audio tests:\n");

    /* WAV parser tests */
    RUN(test_wav_parse_mono8);
    RUN(test_wav_parse_stereo16);
    RUN(test_wav_parse_mono16);
    RUN(test_wav_parse_null_args);
    RUN(test_wav_parse_truncated);
    RUN(test_wav_parse_bad_magic);
    RUN(test_wav_parse_bad_wave);

    /* Audio engine tests */
    RUN(test_init_shutdown);
    RUN(test_double_init);
    RUN(test_shutdown_without_init);
    RUN(test_set_master_volume);
    RUN(test_set_listener);
    RUN(test_tick_no_crash);
    RUN(test_stop_music_no_crash);
    RUN(test_play_invalid_sound);
    RUN(test_operations_without_init);

    printf("\n  %d/%d tests passed\n", g_tests_passed, g_tests_run);
    return 0;
}
