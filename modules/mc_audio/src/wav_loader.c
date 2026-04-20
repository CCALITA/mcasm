#include "audio_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Minimal WAV parser supporting:
 *   - PCM format (format code 1)
 *   - 8-bit and 16-bit samples
 *   - Mono and stereo
 */

static int mem_read_u32_le(const uint8_t *p, uint32_t *out)
{
    *out = (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
    return 4;
}

static int mem_read_u16_le(const uint8_t *p, uint16_t *out)
{
    *out = (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
    return 2;
}

mc_error_t wav_parse(const void *buf, size_t buf_size, wav_data_t *wav_out)
{
    if (!buf || !wav_out) {
        return MC_ERR_INVALID_ARG;
    }

    /* Minimum WAV header: 44 bytes for RIFF + fmt + data headers */
    if (buf_size < 44) {
        return MC_ERR_IO;
    }

    const uint8_t *p = (const uint8_t *)buf;
    size_t offset = 0;

    /* RIFF header */
    if (memcmp(p + offset, "RIFF", 4) != 0) {
        return MC_ERR_IO;
    }
    offset += 4;

    uint32_t riff_size;
    offset += (size_t)mem_read_u32_le(p + offset, &riff_size);
    (void)riff_size;

    if (memcmp(p + offset, "WAVE", 4) != 0) {
        return MC_ERR_IO;
    }
    offset += 4;

    /* Walk chunks to find "fmt " and "data" */
    uint16_t audio_format = 0;
    uint16_t channels = 0;
    uint32_t sample_rate = 0;
    uint16_t bits_per_sample = 0;
    int found_fmt = 0;

    const uint8_t *pcm_data = NULL;
    uint32_t pcm_data_size = 0;

    while (offset + 8 <= buf_size) {
        const uint8_t *chunk_id = p + offset;
        offset += 4;

        uint32_t chunk_size;
        offset += (size_t)mem_read_u32_le(p + offset, &chunk_size);

        if (offset + chunk_size > buf_size) {
            return MC_ERR_IO;
        }

        if (memcmp(chunk_id, "fmt ", 4) == 0) {
            if (chunk_size < 16) {
                return MC_ERR_IO;
            }
            size_t fmt_off = offset;
            fmt_off += (size_t)mem_read_u16_le(p + fmt_off, &audio_format);
            fmt_off += (size_t)mem_read_u16_le(p + fmt_off, &channels);
            fmt_off += (size_t)mem_read_u32_le(p + fmt_off, &sample_rate);
            fmt_off += 4; /* skip byte rate */
            fmt_off += 2; /* skip block align */
            mem_read_u16_le(p + fmt_off, &bits_per_sample);
            found_fmt = 1;
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            pcm_data = p + offset;
            pcm_data_size = chunk_size;
        }

        /* Advance to next chunk (chunks are 2-byte aligned) */
        offset += chunk_size;
        if (chunk_size & 1) {
            offset++;
        }

        if (found_fmt && pcm_data) {
            break;
        }
    }

    if (!found_fmt || !pcm_data || pcm_data_size == 0) {
        return MC_ERR_IO;
    }

    /* Only support PCM format */
    if (audio_format != 1) {
        return MC_ERR_IO;
    }

    /* Only support 8-bit or 16-bit */
    if (bits_per_sample != 8 && bits_per_sample != 16) {
        return MC_ERR_IO;
    }

    /* Only support mono or stereo */
    if (channels != 1 && channels != 2) {
        return MC_ERR_IO;
    }

    void *data_copy = malloc(pcm_data_size);
    if (!data_copy) {
        return MC_ERR_OUT_OF_MEMORY;
    }
    memcpy(data_copy, pcm_data, pcm_data_size);

    wav_out->data = data_copy;
    wav_out->data_size = pcm_data_size;
    wav_out->channels = channels;
    wav_out->bits_per_sample = bits_per_sample;
    wav_out->sample_rate = sample_rate;

    return MC_OK;
}

mc_error_t wav_load_file(const char *path, wav_data_t *wav_out)
{
    if (!path || !wav_out) {
        return MC_ERR_INVALID_ARG;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        return MC_ERR_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(f);
        return MC_ERR_IO;
    }

    void *buf = malloc((size_t)file_size);
    if (!buf) {
        fclose(f);
        return MC_ERR_OUT_OF_MEMORY;
    }

    size_t read_bytes = fread(buf, 1, (size_t)file_size, f);
    fclose(f);

    if (read_bytes != (size_t)file_size) {
        free(buf);
        return MC_ERR_IO;
    }

    mc_error_t err = wav_parse(buf, (size_t)file_size, wav_out);
    free(buf);
    return err;
}
