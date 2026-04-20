#include "mc_audio.h"
#include "audio_internal.h"

/*
 * Compile-time OpenAL detection:
 *   macOS:        always available via -framework OpenAL
 *   Linux/other:  check for HAS_OPENAL define (set by Makefile)
 *
 * Build with -DHAS_OPENAL=1 to enable, or -DHAS_OPENAL=0 to force stubs.
 */
#if defined(__APPLE__)
  #ifndef HAS_OPENAL
    #define HAS_OPENAL 1
  #endif
#else
  #ifndef HAS_OPENAL
    #define HAS_OPENAL 0
  #endif
#endif

#if HAS_OPENAL

#ifdef __APPLE__
  /* Apple deprecated OpenAL in favour of AVAudioEngine, but it still works. */
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  #include <OpenAL/al.h>
  #include <OpenAL/alc.h>
#else
  #include <AL/al.h>
  #include <AL/alc.h>
#endif

#include <stdlib.h>
#include <string.h>

/* ------- constants ------- */

#define MAX_SOUNDS    256
#define MAX_SOURCES   64

/* ------- state ------- */

typedef struct {
    ALCdevice  *device;
    ALCcontext *context;

    /* Sound buffers (slot 0 unused; handle = index) */
    ALuint      buffers[MAX_SOUNDS];
    uint32_t    buffer_count;

    /* Active one-shot sources */
    ALuint      sources[MAX_SOURCES];
    uint32_t    source_count;

    /* Background music source */
    ALuint      music_source;
    int         music_playing;

    float       master_volume;
    int         initialized;
} audio_state_t;

static audio_state_t g_audio;

/* ------- helpers ------- */

static ALenum wav_to_al_format(uint16_t channels, uint16_t bits)
{
    if (channels == 1 && bits == 8)  return AL_FORMAT_MONO8;
    if (channels == 1 && bits == 16) return AL_FORMAT_MONO16;
    if (channels == 2 && bits == 8)  return AL_FORMAT_STEREO8;
    if (channels == 2 && bits == 16) return AL_FORMAT_STEREO16;
    return 0;
}

/* ------- internal helpers ------- */

/*
 * Load a WAV file from disk, create an AL buffer, and return the buffer ID.
 * Returns 0 on failure.  Caller owns the AL buffer.
 */
static ALuint load_wav_into_al_buffer(const char *path)
{
    wav_data_t wav;
    if (wav_load_file(path, &wav) != MC_OK) {
        return 0;
    }

    ALenum format = wav_to_al_format(wav.channels, wav.bits_per_sample);
    if (format == 0) {
        free(wav.data);
        return 0;
    }

    ALuint buf;
    alGenBuffers(1, &buf);
    alBufferData(buf, format, wav.data, (ALsizei)wav.data_size,
                 (ALsizei)wav.sample_rate);
    free(wav.data);

    if (alGetError() != AL_NO_ERROR) {
        alDeleteBuffers(1, &buf);
        return 0;
    }
    return buf;
}

/* ------- public API ------- */

mc_error_t mc_audio_init(void)
{
    if (g_audio.initialized) {
        return MC_OK;
    }

    memset(&g_audio, 0, sizeof(g_audio));
    g_audio.master_volume = 1.0f;

    g_audio.device = alcOpenDevice(NULL);
    if (!g_audio.device) {
        return MC_ERR_PLATFORM;
    }

    g_audio.context = alcCreateContext(g_audio.device, NULL);
    if (!g_audio.context) {
        alcCloseDevice(g_audio.device);
        g_audio.device = NULL;
        return MC_ERR_PLATFORM;
    }

    alcMakeContextCurrent(g_audio.context);

    /* Set default listener at origin */
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    float ori[6] = { 0.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f };
    alListenerfv(AL_ORIENTATION, ori);
    alListenerf(AL_GAIN, g_audio.master_volume);

    /* Reserve slot 0 (invalid handle) */
    g_audio.buffer_count = 1;

    g_audio.initialized = 1;
    return MC_OK;
}

void mc_audio_shutdown(void)
{
    if (!g_audio.initialized) {
        return;
    }

    /* Stop and delete music source */
    if (g_audio.music_playing) {
        alSourceStop(g_audio.music_source);
        alDeleteSources(1, &g_audio.music_source);
    }

    /* Delete active one-shot sources */
    for (uint32_t i = 0; i < g_audio.source_count; i++) {
        alSourceStop(g_audio.sources[i]);
        alDeleteSources(1, &g_audio.sources[i]);
    }

    /* Delete sound buffers (skip slot 0) */
    for (uint32_t i = 1; i < g_audio.buffer_count; i++) {
        if (g_audio.buffers[i]) {
            alDeleteBuffers(1, &g_audio.buffers[i]);
        }
    }

    alcMakeContextCurrent(NULL);
    if (g_audio.context) {
        alcDestroyContext(g_audio.context);
    }
    if (g_audio.device) {
        alcCloseDevice(g_audio.device);
    }

    memset(&g_audio, 0, sizeof(g_audio));
}

uint32_t mc_audio_load_sound(const char *path)
{
    if (!g_audio.initialized || !path) {
        return 0;
    }
    if (g_audio.buffer_count >= MAX_SOUNDS) {
        return 0;
    }

    ALuint buf = load_wav_into_al_buffer(path);
    if (!buf) {
        return 0;
    }

    uint32_t handle = g_audio.buffer_count;
    g_audio.buffers[handle] = buf;
    g_audio.buffer_count++;
    return handle;
}

void mc_audio_play(uint32_t sound_id, vec3_t position, float volume, float pitch)
{
    if (!g_audio.initialized) {
        return;
    }
    if (sound_id == 0 || sound_id >= g_audio.buffer_count) {
        return;
    }
    if (g_audio.source_count >= MAX_SOURCES) {
        /* No room -- caller can retry next tick after cleanup */
        return;
    }

    ALuint src;
    alGenSources(1, &src);
    if (alGetError() != AL_NO_ERROR) {
        return;
    }

    alSourcei(src, AL_BUFFER, (ALint)g_audio.buffers[sound_id]);
    alSource3f(src, AL_POSITION, position.x, position.y, position.z);
    alSourcef(src, AL_GAIN, volume * g_audio.master_volume);
    alSourcef(src, AL_PITCH, pitch);
    alSourcei(src, AL_LOOPING, AL_FALSE);
    alSourcePlay(src);

    g_audio.sources[g_audio.source_count] = src;
    g_audio.source_count++;
}

void mc_audio_play_music(const char *path, float volume)
{
    if (!g_audio.initialized || !path) {
        return;
    }
    if (g_audio.buffer_count >= MAX_SOUNDS) {
        return;
    }

    mc_audio_stop_music();

    ALuint buf = load_wav_into_al_buffer(path);
    if (!buf) {
        return;
    }

    /* Track the buffer so shutdown cleans it up */
    g_audio.buffers[g_audio.buffer_count] = buf;
    g_audio.buffer_count++;

    alGenSources(1, &g_audio.music_source);
    alSourcei(g_audio.music_source, AL_BUFFER, (ALint)buf);
    alSourcef(g_audio.music_source, AL_GAIN, volume * g_audio.master_volume);
    alSourcei(g_audio.music_source, AL_LOOPING, AL_TRUE);
    alSourcei(g_audio.music_source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(g_audio.music_source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSourcePlay(g_audio.music_source);

    g_audio.music_playing = 1;
}

void mc_audio_stop_music(void)
{
    if (!g_audio.initialized || !g_audio.music_playing) {
        return;
    }

    alSourceStop(g_audio.music_source);
    alDeleteSources(1, &g_audio.music_source);
    g_audio.music_source = 0;
    g_audio.music_playing = 0;
}

void mc_audio_set_listener(vec3_t position, vec3_t forward, vec3_t up)
{
    if (!g_audio.initialized) {
        return;
    }

    alListener3f(AL_POSITION, position.x, position.y, position.z);

    float ori[6] = {
        forward.x, forward.y, forward.z,
        up.x,      up.y,      up.z
    };
    alListenerfv(AL_ORIENTATION, ori);
}

void mc_audio_set_master_volume(float volume)
{
    if (!g_audio.initialized) {
        return;
    }

    g_audio.master_volume = (volume < 0.0f) ? 0.0f
                          : (volume > 1.0f) ? 1.0f
                          : volume;
    alListenerf(AL_GAIN, g_audio.master_volume);
}

void mc_audio_tick(void)
{
    if (!g_audio.initialized) {
        return;
    }

    /* Compact finished one-shot sources */
    uint32_t write = 0;
    for (uint32_t i = 0; i < g_audio.source_count; i++) {
        ALint state;
        alGetSourcei(g_audio.sources[i], AL_SOURCE_STATE, &state);
        if (state == AL_STOPPED) {
            alDeleteSources(1, &g_audio.sources[i]);
        } else {
            g_audio.sources[write] = g_audio.sources[i];
            write++;
        }
    }
    g_audio.source_count = write;
}

#ifdef __APPLE__
#pragma GCC diagnostic pop
#endif

#else /* HAS_OPENAL == 0 : stub implementation */

mc_error_t mc_audio_init(void)                   { return MC_OK; }
void       mc_audio_shutdown(void)               {}
uint32_t   mc_audio_load_sound(const char *p)    { (void)p; return 0; }
void       mc_audio_play(uint32_t id, vec3_t pos, float vol, float pitch)
           { (void)id; (void)pos; (void)vol; (void)pitch; }
void       mc_audio_play_music(const char *p, float v) { (void)p; (void)v; }
void       mc_audio_stop_music(void)             {}
void       mc_audio_set_listener(vec3_t p, vec3_t f, vec3_t u)
           { (void)p; (void)f; (void)u; }
void       mc_audio_set_master_volume(float v)   { (void)v; }
void       mc_audio_tick(void)                   {}

#endif /* HAS_OPENAL */
