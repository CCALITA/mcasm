#ifndef MC_AUDIO_INTERNAL_H
#define MC_AUDIO_INTERNAL_H

#include "mc_types.h"
#include "mc_error.h"
#include <stdint.h>
#include <stddef.h>

/* WAV loader result */
typedef struct {
    void    *data;
    uint32_t data_size;
    uint16_t channels;
    uint16_t bits_per_sample;
    uint32_t sample_rate;
} wav_data_t;

/*
 * Parse a WAV file from a memory buffer.
 * On success, wav_out->data points to malloc'd PCM data (caller must free).
 * Returns MC_OK on success, MC_ERR_IO on parse failure.
 */
mc_error_t wav_parse(const void *buf, size_t buf_size, wav_data_t *wav_out);

/*
 * Load a WAV file from disk.
 * On success, wav_out->data points to malloc'd PCM data (caller must free).
 * Returns MC_OK on success, error code otherwise.
 */
mc_error_t wav_load_file(const char *path, wav_data_t *wav_out);

#endif /* MC_AUDIO_INTERNAL_H */
