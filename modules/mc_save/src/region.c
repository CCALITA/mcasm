/*
 * region.c -- Anvil region file format.
 *
 * Each region file stores a 32x32 grid of chunks.
 * Layout:
 *   - 4 KB offset/sector-count table  (1024 entries x 4 bytes)
 *   - 4 KB timestamp table            (1024 entries x 4 bytes)
 *   - Chunk data in 4 KB sectors, each prefixed by:
 *       [4 bytes: payload length] [1 byte: compression type] [payload]
 *
 * Compression type 0 = uncompressed NBT (we use this for simplicity).
 */

#include "nbt.h"
#include "save_internal.h"
#include "mc_save.h"
#include "mc_types.h"
#include "mc_error.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Compression type: uncompressed. */
#define REGION_COMPRESS_NONE 0

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

/* Offset into the 1024-entry region tables. */
static int region_index(int local_x, int local_z)
{
    return (local_z & (REGION_CHUNKS_Z - 1)) * REGION_CHUNKS_X + (local_x & (REGION_CHUNKS_X - 1));
}

/* Build the region file path for a given chunk position. */
static mc_error_t region_path(chunk_pos_t pos, char* buf, size_t buf_size)
{
    int32_t rx = floor_div(pos.x, REGION_CHUNKS_X);
    int32_t rz = floor_div(pos.z, REGION_CHUNKS_Z);
    return save_build_path(buf, buf_size, "%s/region/r.%d.%d.mca",
                           g_world_dir, rx, rz);
}

/* Pack offset (sector number) and sector count into 4 bytes.
 * Bits [31..8] = sector offset, bits [7..0] = sector count. */
static uint32_t pack_offset(uint32_t sector, uint8_t count)
{
    return (sector << 8) | count;
}

static void unpack_offset(uint32_t packed, uint32_t* sector, uint8_t* count)
{
    *sector = packed >> 8;
    *count  = (uint8_t)(packed & 0xFF);
}

/* Write a uint32 in big-endian at arbitrary byte offset in a buffer. */
static void put_be32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v);
}

static uint32_t get_be32(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | ((uint32_t)p[3]);
}

/* ------------------------------------------------------------------ */
/*  Chunk <-> NBT serialization                                        */
/* ------------------------------------------------------------------ */

static nbt_tag_t* chunk_to_nbt(chunk_pos_t pos, const chunk_t* chunk)
{
    nbt_tag_t* root = nbt_create_compound("Chunk");
    if (!root) return NULL;

    if (nbt_compound_add(root, nbt_create_int("xPos", pos.x)) != MC_OK ||
        nbt_compound_add(root, nbt_create_int("zPos", pos.z)) != MC_OK ||
        nbt_compound_add(root, nbt_create_byte("state", (int8_t)chunk->state)) != MC_OK ||
        nbt_compound_add(root, nbt_create_int("dirtyMask", (int32_t)chunk->dirty_mask)) != MC_OK) {
        nbt_free(root);
        return NULL;
    }

    nbt_tag_t* hm = nbt_create_int_array("Heightmap", chunk->heightmap,
                                          CHUNK_SIZE_X * CHUNK_SIZE_Z);
    if (!hm || nbt_compound_add(root, hm) != MC_OK) {
        nbt_free(hm);
        nbt_free(root);
        return NULL;
    }

    nbt_tag_t* sections = nbt_create_list("Sections", NBT_TAG_COMPOUND);
    if (!sections) { nbt_free(root); return NULL; }

    for (int s = 0; s < SECTION_COUNT; s++) {
        const chunk_section_t* sec = &chunk->sections[s];
        if (sec->non_air_count == 0) continue;

        nbt_tag_t* stag = nbt_create_compound(NULL);
        if (!stag) { nbt_free(sections); nbt_free(root); return NULL; }

        if (nbt_compound_add(stag, nbt_create_byte("Y", (int8_t)s)) != MC_OK ||
            nbt_compound_add(stag, nbt_create_short("nonAirCount", (int16_t)sec->non_air_count)) != MC_OK ||
            nbt_compound_add(stag,
                nbt_create_byte_array("Blocks",
                                      (const int8_t*)sec->blocks,
                                      BLOCKS_PER_SECTION * (int32_t)sizeof(block_id_t))) != MC_OK ||
            nbt_compound_add(stag,
                nbt_create_byte_array("BlockLight",
                                      (const int8_t*)sec->block_light,
                                      BLOCKS_PER_SECTION / 2)) != MC_OK ||
            nbt_compound_add(stag,
                nbt_create_byte_array("SkyLight",
                                      (const int8_t*)sec->sky_light,
                                      BLOCKS_PER_SECTION / 2)) != MC_OK) {
            nbt_free(stag);
            nbt_free(sections);
            nbt_free(root);
            return NULL;
        }

        if (nbt_list_append(sections, stag) != MC_OK) {
            nbt_free(stag);
            nbt_free(sections);
            nbt_free(root);
            return NULL;
        }
    }

    if (nbt_compound_add(root, sections) != MC_OK) {
        nbt_free(sections);
        nbt_free(root);
        return NULL;
    }
    return root;
}

static mc_error_t nbt_to_chunk(const nbt_tag_t* root, chunk_t* out)
{
    if (!root || root->type != NBT_TAG_COMPOUND) return MC_ERR_IO;

    memset(out, 0, sizeof(chunk_t));

    nbt_tag_t* xp = nbt_compound_get(root, "xPos");
    nbt_tag_t* zp = nbt_compound_get(root, "zPos");
    if (xp) out->pos.x = xp->payload.v_int;
    if (zp) out->pos.z = zp->payload.v_int;

    nbt_tag_t* st = nbt_compound_get(root, "state");
    if (st) out->state = (uint8_t)st->payload.v_byte;

    nbt_tag_t* dm = nbt_compound_get(root, "dirtyMask");
    if (dm) out->dirty_mask = (uint32_t)dm->payload.v_int;

    /* Heightmap. */
    nbt_tag_t* hm = nbt_compound_get(root, "Heightmap");
    if (hm && hm->type == NBT_TAG_INT_ARRAY) {
        int32_t count = hm->payload.v_int_array.length;
        if (count > CHUNK_SIZE_X * CHUNK_SIZE_Z)
            count = CHUNK_SIZE_X * CHUNK_SIZE_Z;
        memcpy(out->heightmap, hm->payload.v_int_array.data,
               (size_t)count * sizeof(int32_t));
    }

    /* Sections. */
    nbt_tag_t* secs = nbt_compound_get(root, "Sections");
    if (secs && secs->type == NBT_TAG_LIST) {
        for (int32_t i = 0; i < secs->payload.v_list.count; i++) {
            nbt_tag_t* stag = secs->payload.v_list.items[i];
            if (!stag || stag->type != NBT_TAG_COMPOUND) continue;

            nbt_tag_t* y_tag = nbt_compound_get(stag, "Y");
            if (!y_tag) continue;
            int y = (int)y_tag->payload.v_byte;
            if (y < 0 || y >= SECTION_COUNT) continue;

            chunk_section_t* sec = &out->sections[y];

            nbt_tag_t* nac = nbt_compound_get(stag, "nonAirCount");
            if (nac) sec->non_air_count = (uint16_t)nac->payload.v_short;

            nbt_tag_t* blk = nbt_compound_get(stag, "Blocks");
            if (blk && blk->type == NBT_TAG_BYTE_ARRAY) {
                int32_t expect = BLOCKS_PER_SECTION * (int32_t)sizeof(block_id_t);
                if (blk->payload.v_byte_array.length == expect)
                    memcpy(sec->blocks, blk->payload.v_byte_array.data, (size_t)expect);
            }

            nbt_tag_t* bl = nbt_compound_get(stag, "BlockLight");
            if (bl && bl->type == NBT_TAG_BYTE_ARRAY &&
                bl->payload.v_byte_array.length == BLOCKS_PER_SECTION / 2)
                memcpy(sec->block_light, bl->payload.v_byte_array.data,
                       BLOCKS_PER_SECTION / 2);

            nbt_tag_t* sl = nbt_compound_get(stag, "SkyLight");
            if (sl && sl->type == NBT_TAG_BYTE_ARRAY &&
                sl->payload.v_byte_array.length == BLOCKS_PER_SECTION / 2)
                memcpy(sec->sky_light, sl->payload.v_byte_array.data,
                       BLOCKS_PER_SECTION / 2);
        }
    }

    return MC_OK;
}

/* ------------------------------------------------------------------ */
/*  Region file read/write                                             */
/* ------------------------------------------------------------------ */

/* Read or create a region file, returning a malloc'd buffer of its contents.
 * *out_size receives the actual file size (which may be exactly REGION_HEADER_SIZE
 * for a freshly created file). */
static mc_error_t region_open(const char* path, uint8_t** out, size_t* out_size)
{
    FILE* f = fopen(path, "rb");
    if (!f) {
        /* Brand new region -- create an empty header. */
        *out = calloc(1, REGION_HEADER_SIZE);
        if (!*out) return MC_ERR_OUT_OF_MEMORY;
        *out_size = REGION_HEADER_SIZE;
        return MC_OK;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < (long)REGION_HEADER_SIZE) {
        fclose(f);
        *out = calloc(1, REGION_HEADER_SIZE);
        if (!*out) return MC_ERR_OUT_OF_MEMORY;
        *out_size = REGION_HEADER_SIZE;
        return MC_OK;
    }

    *out = malloc((size_t)fsize);
    if (!*out) { fclose(f); return MC_ERR_OUT_OF_MEMORY; }
    if (fread(*out, 1, (size_t)fsize, f) != (size_t)fsize) {
        free(*out);
        fclose(f);
        return MC_ERR_IO;
    }
    fclose(f);
    *out_size = (size_t)fsize;
    return MC_OK;
}

/* Find the first free run of `need` sectors starting at sector 2. */
static uint32_t region_find_free(const uint8_t* header, size_t file_sectors,
                                 uint32_t need)
{
    /* Build a simple occupancy bitmap. Sectors 0-1 are always occupied (header). */
    /* Scan each entry in the offset table; mark occupied ranges. */
    uint32_t max_sector = (uint32_t)(file_sectors > 0 ? file_sectors : REGION_HEADER_SECTORS);

    /* Allocate a boolean array for occupancy. */
    uint32_t alloc_size = max_sector + need + 1;
    uint8_t* used = calloc(alloc_size, 1);
    if (!used) return max_sector; /* fall through: append at end */

    used[0] = 1;
    used[1] = 1;

    for (int i = 0; i < 1024; i++) {
        uint32_t packed = get_be32(header + i * 4);
        uint32_t sec;
        uint8_t cnt;
        unpack_offset(packed, &sec, &cnt);
        if (sec >= REGION_HEADER_SECTORS && cnt > 0) {
            for (uint32_t s = sec; s < sec + cnt && s < alloc_size; s++)
                used[s] = 1;
        }
    }

    /* Linear scan for a free run. */
    uint32_t run = 0;
    for (uint32_t s = REGION_HEADER_SECTORS; s < alloc_size; s++) {
        if (!used[s]) {
            run++;
            if (run >= need) { free(used); return s - need + 1; }
        } else {
            run = 0;
        }
    }

    free(used);
    return max_sector; /* append at end of file */
}

mc_error_t mc_save_chunk(chunk_pos_t pos, const chunk_t* chunk)
{
    if (!chunk) return MC_ERR_INVALID_ARG;
    if (g_world_dir[0] == '\0') return MC_ERR_IO;

    char path[SAVE_MAX_PATH];
    mc_error_t err = region_path(pos, path, sizeof(path));
    if (err) return err;

    /* Serialize chunk to NBT, then to binary. */
    nbt_tag_t* nbt = chunk_to_nbt(pos, chunk);
    if (!nbt) return MC_ERR_OUT_OF_MEMORY;

    nbt_writer_t nbt_buf;
    nbt_writer_init(&nbt_buf);
    err = nbt_write(&nbt_buf, nbt);
    nbt_free(nbt);
    if (err) { nbt_writer_free(&nbt_buf); return err; }

    /* Chunk entry: [4-byte length][1-byte compression type][payload]. */
    size_t entry_size = 4 + 1 + nbt_buf.size;
    /* Round up to sector boundary. */
    uint32_t sectors_needed = (uint32_t)((entry_size + REGION_SECTOR_SIZE - 1) / REGION_SECTOR_SIZE);

    /* Load or create region file. */
    uint8_t* region = NULL;
    size_t region_size = 0;
    err = region_open(path, &region, &region_size);
    if (err) { nbt_writer_free(&nbt_buf); return err; }

    size_t file_sectors = region_size / REGION_SECTOR_SIZE;
    int idx = region_index(pos.x & (REGION_CHUNKS_X - 1), pos.z & (REGION_CHUNKS_Z - 1));

    /* Find space for the chunk. */
    uint32_t target_sector = region_find_free(region, file_sectors, sectors_needed);

    /* Ensure the buffer is big enough. */
    size_t needed_size = ((size_t)target_sector + sectors_needed) * REGION_SECTOR_SIZE;
    if (needed_size > region_size) {
        uint8_t* new_region = realloc(region, needed_size);
        if (!new_region) { free(region); nbt_writer_free(&nbt_buf); return MC_ERR_OUT_OF_MEMORY; }
        memset(new_region + region_size, 0, needed_size - region_size);
        region = new_region;
        region_size = needed_size;
    }

    /* Write chunk data into the target sectors. */
    uint8_t* dst = region + (size_t)target_sector * REGION_SECTOR_SIZE;
    put_be32(dst, (uint32_t)(nbt_buf.size + 1));  /* +1 for compression byte */
    dst[4] = REGION_COMPRESS_NONE;
    memcpy(dst + 5, nbt_buf.data, nbt_buf.size);
    nbt_writer_free(&nbt_buf);

    /* Update offset table. */
    put_be32(region + idx * 4, pack_offset(target_sector, (uint8_t)sectors_needed));

    /* Update timestamp table. */
    put_be32(region + 4096 + idx * 4, (uint32_t)time(NULL));

    /* Write region file. */
    FILE* f = fopen(path, "wb");
    if (!f) { free(region); return MC_ERR_IO; }
    size_t written = fwrite(region, 1, region_size, f);
    fclose(f);
    free(region);

    return (written == region_size) ? MC_OK : MC_ERR_IO;
}

mc_error_t mc_save_load_chunk(chunk_pos_t pos, chunk_t* out_chunk)
{
    if (!out_chunk) return MC_ERR_INVALID_ARG;
    if (g_world_dir[0] == '\0') return MC_ERR_IO;

    char path[SAVE_MAX_PATH];
    mc_error_t err = region_path(pos, path, sizeof(path));
    if (err) return err;

    FILE* f = fopen(path, "rb");
    if (!f) return MC_ERR_NOT_FOUND;

    /* Read offset table entry. */
    int idx = region_index(pos.x & (REGION_CHUNKS_X - 1), pos.z & (REGION_CHUNKS_Z - 1));
    uint8_t entry_bytes[4];
    fseek(f, idx * 4, SEEK_SET);
    if (fread(entry_bytes, 1, 4, f) != 4) { fclose(f); return MC_ERR_IO; }

    uint32_t packed = get_be32(entry_bytes);
    uint32_t sector;
    uint8_t count;
    unpack_offset(packed, &sector, &count);

    if (sector < REGION_HEADER_SECTORS || count == 0) {
        fclose(f);
        return MC_ERR_NOT_FOUND;
    }

    /* Read chunk header (length + compression type). */
    size_t data_offset = (size_t)sector * REGION_SECTOR_SIZE;
    fseek(f, (long)data_offset, SEEK_SET);

    uint8_t hdr[5];
    if (fread(hdr, 1, 5, f) != 5) { fclose(f); return MC_ERR_IO; }

    uint32_t payload_len = get_be32(hdr);
    if (payload_len <= 1) { fclose(f); return MC_ERR_IO; }
    /* uint8_t compress_type = hdr[4]; -- we only support uncompressed */

    size_t nbt_len = payload_len - 1; /* subtract compression byte */
    uint8_t* nbt_data = malloc(nbt_len);
    if (!nbt_data) { fclose(f); return MC_ERR_OUT_OF_MEMORY; }

    if (fread(nbt_data, 1, nbt_len, f) != nbt_len) {
        free(nbt_data);
        fclose(f);
        return MC_ERR_IO;
    }
    fclose(f);

    /* Parse NBT. */
    nbt_reader_t reader = { .data = nbt_data, .size = nbt_len, .pos = 0 };
    nbt_tag_t* root = NULL;
    err = nbt_read(&reader, &root);
    free(nbt_data);
    if (err) return err;

    err = nbt_to_chunk(root, out_chunk);
    nbt_free(root);
    return err;
}

uint8_t mc_save_chunk_exists(chunk_pos_t pos)
{
    if (g_world_dir[0] == '\0') return 0;

    char path[SAVE_MAX_PATH];
    if (region_path(pos, path, sizeof(path)) != MC_OK) return 0;

    FILE* f = fopen(path, "rb");
    if (!f) return 0;

    int idx = region_index(pos.x & (REGION_CHUNKS_X - 1), pos.z & (REGION_CHUNKS_Z - 1));
    uint8_t entry_bytes[4];
    fseek(f, idx * 4, SEEK_SET);
    if (fread(entry_bytes, 1, 4, f) != 4) { fclose(f); return 0; }
    fclose(f);

    uint32_t packed = get_be32(entry_bytes);
    uint32_t sector;
    uint8_t count;
    unpack_offset(packed, &sector, &count);

    return (sector >= REGION_HEADER_SECTORS && count > 0) ? 1 : 0;
}
