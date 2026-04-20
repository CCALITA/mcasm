/*
 * nbt.c -- Named Binary Tag (NBT) parser/writer.
 *
 * Implements the Minecraft NBT format: big-endian binary encoding of
 * typed, named, nestable tags.  Used by region files, player data, and
 * world metadata.
 */

#include "nbt.h"
#include "save_internal.h"
#include "mc_error.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  Endian helpers (host <-> big-endian)                               */
/* ------------------------------------------------------------------ */

static uint16_t swap16(uint16_t v)
{
    return (uint16_t)((v >> 8) | (v << 8));
}

static uint32_t swap32(uint32_t v)
{
    v = ((v >> 8)  & 0x00FF00FFu) | ((v << 8)  & 0xFF00FF00u);
    v = ((v >> 16))               | ((v << 16));
    return v;
}

static uint64_t swap64(uint64_t v)
{
    v = ((v >>  8) & 0x00FF00FF00FF00FFull) | ((v <<  8) & 0xFF00FF00FF00FF00ull);
    v = ((v >> 16) & 0x0000FFFF0000FFFFull) | ((v << 16) & 0xFFFF0000FFFF0000ull);
    v = ((v >> 32))                         | ((v << 32));
    return v;
}

static int is_big_endian(void)
{
    uint16_t test = 1;
    return *(const uint8_t*)&test == 0;
}

static uint16_t to_big16(uint16_t v) { return is_big_endian() ? v : swap16(v); }
static uint32_t to_big32(uint32_t v) { return is_big_endian() ? v : swap32(v); }
static uint64_t to_big64(uint64_t v) { return is_big_endian() ? v : swap64(v); }

static uint16_t from_big16(uint16_t v) { return to_big16(v); }
static uint32_t from_big32(uint32_t v) { return to_big32(v); }
static uint64_t from_big64(uint64_t v) { return to_big64(v); }

/* ------------------------------------------------------------------ */
/*  Internal read helpers                                              */
/* ------------------------------------------------------------------ */

static mc_error_t reader_read(nbt_reader_t* r, void* dst, size_t n)
{
    if (r->pos + n > r->size) return MC_ERR_IO;
    memcpy(dst, r->data + r->pos, n);
    r->pos += n;
    return MC_OK;
}

static mc_error_t read_u8(nbt_reader_t* r, uint8_t* v)
{
    return reader_read(r, v, 1);
}

static mc_error_t read_i16(nbt_reader_t* r, int16_t* v)
{
    uint16_t raw;
    mc_error_t err = reader_read(r, &raw, 2);
    if (err) return err;
    raw = from_big16(raw);
    *v = (int16_t)raw;
    return MC_OK;
}

static mc_error_t read_i32(nbt_reader_t* r, int32_t* v)
{
    uint32_t raw;
    mc_error_t err = reader_read(r, &raw, 4);
    if (err) return err;
    raw = from_big32(raw);
    *v = (int32_t)raw;
    return MC_OK;
}

static mc_error_t read_i64(nbt_reader_t* r, int64_t* v)
{
    uint64_t raw;
    mc_error_t err = reader_read(r, &raw, 8);
    if (err) return err;
    raw = from_big64(raw);
    *v = (int64_t)raw;
    return MC_OK;
}

static mc_error_t read_f32(nbt_reader_t* r, float* v)
{
    uint32_t raw;
    mc_error_t err = reader_read(r, &raw, 4);
    if (err) return err;
    raw = from_big32(raw);
    memcpy(v, &raw, 4);
    return MC_OK;
}

static mc_error_t read_f64(nbt_reader_t* r, double* v)
{
    uint64_t raw;
    mc_error_t err = reader_read(r, &raw, 8);
    if (err) return err;
    raw = from_big64(raw);
    memcpy(v, &raw, 8);
    return MC_OK;
}

static mc_error_t read_string(nbt_reader_t* r, char** out, uint16_t* out_len)
{
    int16_t slen;
    mc_error_t err = read_i16(r, &slen);
    if (err) return err;
    if (slen < 0) return MC_ERR_IO;

    char* s = malloc((size_t)slen + 1);
    if (!s) return MC_ERR_OUT_OF_MEMORY;

    err = reader_read(r, s, (size_t)slen);
    if (err) { free(s); return err; }
    s[slen] = '\0';

    *out = s;
    if (out_len) *out_len = (uint16_t)slen;
    return MC_OK;
}

/* ------------------------------------------------------------------ */
/*  Internal write helpers                                             */
/* ------------------------------------------------------------------ */

static mc_error_t writer_grow(nbt_writer_t* w, size_t need)
{
    if (w->size + need <= w->capacity) return MC_OK;

    size_t new_cap = w->capacity ? w->capacity : 256;
    while (new_cap < w->size + need) new_cap *= 2;

    uint8_t* new_data = realloc(w->data, new_cap);
    if (!new_data) return MC_ERR_OUT_OF_MEMORY;
    w->data = new_data;
    w->capacity = new_cap;
    return MC_OK;
}

static mc_error_t writer_write(nbt_writer_t* w, const void* src, size_t n)
{
    mc_error_t err = writer_grow(w, n);
    if (err) return err;
    memcpy(w->data + w->size, src, n);
    w->size += n;
    return MC_OK;
}

static mc_error_t write_u8(nbt_writer_t* w, uint8_t v)
{
    return writer_write(w, &v, 1);
}

static mc_error_t write_i16(nbt_writer_t* w, int16_t v)
{
    uint16_t be = to_big16((uint16_t)v);
    return writer_write(w, &be, 2);
}

static mc_error_t write_i32(nbt_writer_t* w, int32_t v)
{
    uint32_t be = to_big32((uint32_t)v);
    return writer_write(w, &be, 4);
}

static mc_error_t write_i64(nbt_writer_t* w, int64_t v)
{
    uint64_t be = to_big64((uint64_t)v);
    return writer_write(w, &be, 8);
}

static mc_error_t write_f32(nbt_writer_t* w, float v)
{
    uint32_t raw;
    memcpy(&raw, &v, 4);
    raw = to_big32(raw);
    return writer_write(w, &raw, 4);
}

static mc_error_t write_f64(nbt_writer_t* w, double v)
{
    uint64_t raw;
    memcpy(&raw, &v, 8);
    raw = to_big64(raw);
    return writer_write(w, &raw, 8);
}

static mc_error_t write_string_raw(nbt_writer_t* w, const char* s, uint16_t len)
{
    mc_error_t err = write_i16(w, (int16_t)len);
    if (err) return err;
    return writer_write(w, s, len);
}

/* ------------------------------------------------------------------ */
/*  Tag allocation helpers                                             */
/* ------------------------------------------------------------------ */

static nbt_tag_t* tag_alloc(uint8_t type, const char* name)
{
    nbt_tag_t* tag = calloc(1, sizeof(nbt_tag_t));
    if (!tag) return NULL;
    tag->type = type;
    if (name) {
        tag->name = strdup(name);
        if (!tag->name) { free(tag); return NULL; }
    }
    return tag;
}

/* ------------------------------------------------------------------ */
/*  Public constructors                                                */
/* ------------------------------------------------------------------ */

nbt_tag_t* nbt_create_byte(const char* name, int8_t value)
{
    nbt_tag_t* t = tag_alloc(NBT_TAG_BYTE, name);
    if (t) t->payload.v_byte = value;
    return t;
}

nbt_tag_t* nbt_create_short(const char* name, int16_t value)
{
    nbt_tag_t* t = tag_alloc(NBT_TAG_SHORT, name);
    if (t) t->payload.v_short = value;
    return t;
}

nbt_tag_t* nbt_create_int(const char* name, int32_t value)
{
    nbt_tag_t* t = tag_alloc(NBT_TAG_INT, name);
    if (t) t->payload.v_int = value;
    return t;
}

nbt_tag_t* nbt_create_long(const char* name, int64_t value)
{
    nbt_tag_t* t = tag_alloc(NBT_TAG_LONG, name);
    if (t) t->payload.v_long = value;
    return t;
}

nbt_tag_t* nbt_create_float(const char* name, float value)
{
    nbt_tag_t* t = tag_alloc(NBT_TAG_FLOAT, name);
    if (t) t->payload.v_float = value;
    return t;
}

nbt_tag_t* nbt_create_double(const char* name, double value)
{
    nbt_tag_t* t = tag_alloc(NBT_TAG_DOUBLE, name);
    if (t) t->payload.v_double = value;
    return t;
}

nbt_tag_t* nbt_create_byte_array(const char* name, const int8_t* data, int32_t len)
{
    if (len < 0 || len > NBT_MAX_ARRAY_LEN) return NULL;
    nbt_tag_t* t = tag_alloc(NBT_TAG_BYTE_ARRAY, name);
    if (!t) return NULL;
    t->payload.v_byte_array.length = len;
    if (len > 0) {
        t->payload.v_byte_array.data = malloc((size_t)len);
        if (!t->payload.v_byte_array.data) { nbt_free(t); return NULL; }
        memcpy(t->payload.v_byte_array.data, data, (size_t)len);
    }
    return t;
}

nbt_tag_t* nbt_create_string(const char* name, const char* value)
{
    if (!value) return NULL;
    size_t slen = strlen(value);
    if (slen > NBT_MAX_STRING_LEN) return NULL;

    nbt_tag_t* t = tag_alloc(NBT_TAG_STRING, name);
    if (!t) return NULL;
    t->payload.v_string.length = (uint16_t)slen;
    t->payload.v_string.data = strdup(value);
    if (!t->payload.v_string.data) { nbt_free(t); return NULL; }
    return t;
}

nbt_tag_t* nbt_create_list(const char* name, uint8_t element_type)
{
    nbt_tag_t* t = tag_alloc(NBT_TAG_LIST, name);
    if (!t) return NULL;
    t->payload.v_list.element_type = element_type;
    return t;
}

nbt_tag_t* nbt_create_compound(const char* name)
{
    return tag_alloc(NBT_TAG_COMPOUND, name);
}

nbt_tag_t* nbt_create_int_array(const char* name, const int32_t* data, int32_t len)
{
    if (len < 0 || len > NBT_MAX_ARRAY_LEN) return NULL;
    nbt_tag_t* t = tag_alloc(NBT_TAG_INT_ARRAY, name);
    if (!t) return NULL;
    t->payload.v_int_array.length = len;
    if (len > 0) {
        t->payload.v_int_array.data = malloc((size_t)len * sizeof(int32_t));
        if (!t->payload.v_int_array.data) { nbt_free(t); return NULL; }
        memcpy(t->payload.v_int_array.data, data, (size_t)len * sizeof(int32_t));
    }
    return t;
}

/* ------------------------------------------------------------------ */
/*  Free                                                               */
/* ------------------------------------------------------------------ */

void nbt_free(nbt_tag_t* tag)
{
    if (!tag) return;

    free(tag->name);

    switch (tag->type) {
    case NBT_TAG_BYTE_ARRAY:
        free(tag->payload.v_byte_array.data);
        break;
    case NBT_TAG_STRING:
        free(tag->payload.v_string.data);
        break;
    case NBT_TAG_LIST:
        for (int32_t i = 0; i < tag->payload.v_list.count; i++)
            nbt_free(tag->payload.v_list.items[i]);
        free(tag->payload.v_list.items);
        break;
    case NBT_TAG_COMPOUND:
        for (int32_t i = 0; i < tag->payload.v_compound.count; i++)
            nbt_free(tag->payload.v_compound.children[i]);
        free(tag->payload.v_compound.children);
        break;
    case NBT_TAG_INT_ARRAY:
        free(tag->payload.v_int_array.data);
        break;
    default:
        break;
    }

    free(tag);
}

/* ------------------------------------------------------------------ */
/*  Compound / List helpers                                            */
/* ------------------------------------------------------------------ */

mc_error_t nbt_compound_add(nbt_tag_t* compound, nbt_tag_t* child)
{
    if (!compound || compound->type != NBT_TAG_COMPOUND || !child)
        return MC_ERR_INVALID_ARG;

    int32_t cnt = compound->payload.v_compound.count;
    int32_t cap = compound->payload.v_compound.capacity;

    if (cnt >= NBT_MAX_CHILDREN) return MC_ERR_FULL;

    if (cnt >= cap) {
        int32_t new_cap = cap ? cap * 2 : 8;
        nbt_tag_t** new_arr = realloc(compound->payload.v_compound.children,
                                      (size_t)new_cap * sizeof(nbt_tag_t*));
        if (!new_arr) return MC_ERR_OUT_OF_MEMORY;
        compound->payload.v_compound.children = new_arr;
        compound->payload.v_compound.capacity = new_cap;
    }

    compound->payload.v_compound.children[cnt] = child;
    compound->payload.v_compound.count = cnt + 1;
    return MC_OK;
}

nbt_tag_t* nbt_compound_get(const nbt_tag_t* compound, const char* name)
{
    if (!compound || compound->type != NBT_TAG_COMPOUND || !name) return NULL;

    for (int32_t i = 0; i < compound->payload.v_compound.count; i++) {
        nbt_tag_t* child = compound->payload.v_compound.children[i];
        if (child->name && strcmp(child->name, name) == 0)
            return child;
    }
    return NULL;
}

mc_error_t nbt_list_append(nbt_tag_t* list, nbt_tag_t* item)
{
    if (!list || list->type != NBT_TAG_LIST || !item)
        return MC_ERR_INVALID_ARG;

    if (item->type != list->payload.v_list.element_type)
        return MC_ERR_INVALID_ARG;

    int32_t cnt = list->payload.v_list.count;
    int32_t cap = list->payload.v_list.capacity;

    if (cnt >= NBT_MAX_CHILDREN) return MC_ERR_FULL;

    if (cnt >= cap) {
        int32_t new_cap = cap ? cap * 2 : 8;
        nbt_tag_t** new_arr = realloc(list->payload.v_list.items,
                                      (size_t)new_cap * sizeof(nbt_tag_t*));
        if (!new_arr) return MC_ERR_OUT_OF_MEMORY;
        list->payload.v_list.items = new_arr;
        list->payload.v_list.capacity = new_cap;
    }

    list->payload.v_list.items[cnt] = item;
    list->payload.v_list.count = cnt + 1;
    return MC_OK;
}

/* ------------------------------------------------------------------ */
/*  Binary read (recursive)                                            */
/* ------------------------------------------------------------------ */

static mc_error_t nbt_read_payload(nbt_reader_t* r, uint8_t type,
                                   nbt_tag_t* tag, int depth);

static mc_error_t nbt_read_tag(nbt_reader_t* r, nbt_tag_t** out, int depth)
{
    if (depth > NBT_MAX_DEPTH) return MC_ERR_IO;

    uint8_t type;
    mc_error_t err = read_u8(r, &type);
    if (err) return err;

    if (type == NBT_TAG_END) {
        *out = NULL;
        return MC_OK;
    }

    char* name = NULL;
    uint16_t name_len = 0;
    err = read_string(r, &name, &name_len);
    if (err) return err;

    nbt_tag_t* tag = tag_alloc(type, NULL);
    if (!tag) { free(name); return MC_ERR_OUT_OF_MEMORY; }
    /* Transfer ownership of name directly (tag->name is NULL from tag_alloc). */
    tag->name = name;

    err = nbt_read_payload(r, type, tag, depth);
    if (err) { nbt_free(tag); return err; }

    *out = tag;
    return MC_OK;
}

static mc_error_t nbt_read_payload(nbt_reader_t* r, uint8_t type,
                                   nbt_tag_t* tag, int depth)
{
    mc_error_t err;

    switch (type) {
    case NBT_TAG_BYTE: {
        uint8_t v;
        err = read_u8(r, &v);
        if (err) return err;
        tag->payload.v_byte = (int8_t)v;
        return MC_OK;
    }
    case NBT_TAG_SHORT:
        return read_i16(r, &tag->payload.v_short);

    case NBT_TAG_INT:
        return read_i32(r, &tag->payload.v_int);

    case NBT_TAG_LONG:
        return read_i64(r, &tag->payload.v_long);

    case NBT_TAG_FLOAT:
        return read_f32(r, &tag->payload.v_float);

    case NBT_TAG_DOUBLE:
        return read_f64(r, &tag->payload.v_double);

    case NBT_TAG_BYTE_ARRAY: {
        int32_t len;
        err = read_i32(r, &len);
        if (err) return err;
        if (len < 0 || len > NBT_MAX_ARRAY_LEN) return MC_ERR_IO;
        tag->payload.v_byte_array.length = len;
        if (len > 0) {
            tag->payload.v_byte_array.data = malloc((size_t)len);
            if (!tag->payload.v_byte_array.data) return MC_ERR_OUT_OF_MEMORY;
            err = reader_read(r, tag->payload.v_byte_array.data, (size_t)len);
            if (err) return err;
        }
        return MC_OK;
    }
    case NBT_TAG_STRING: {
        char* s = NULL;
        uint16_t slen = 0;
        err = read_string(r, &s, &slen);
        if (err) return err;
        tag->payload.v_string.data = s;
        tag->payload.v_string.length = slen;
        return MC_OK;
    }
    case NBT_TAG_LIST: {
        uint8_t elem_type;
        err = read_u8(r, &elem_type);
        if (err) return err;
        int32_t count;
        err = read_i32(r, &count);
        if (err) return err;
        if (count < 0 || count > NBT_MAX_CHILDREN) return MC_ERR_IO;

        tag->payload.v_list.element_type = elem_type;
        if (count > 0) {
            tag->payload.v_list.items = calloc((size_t)count, sizeof(nbt_tag_t*));
            if (!tag->payload.v_list.items) return MC_ERR_OUT_OF_MEMORY;
            tag->payload.v_list.capacity = count;
        }
        for (int32_t i = 0; i < count; i++) {
            nbt_tag_t* item = tag_alloc(elem_type, NULL);
            if (!item) return MC_ERR_OUT_OF_MEMORY;
            err = nbt_read_payload(r, elem_type, item, depth + 1);
            if (err) { nbt_free(item); return err; }
            tag->payload.v_list.items[i] = item;
            tag->payload.v_list.count = i + 1;
        }
        return MC_OK;
    }
    case NBT_TAG_COMPOUND: {
        for (;;) {
            uint8_t child_type;
            err = read_u8(r, &child_type);
            if (err) return err;
            if (child_type == NBT_TAG_END) break;

            char* child_name = NULL;
            err = read_string(r, &child_name, NULL);
            if (err) return err;

            nbt_tag_t* child = tag_alloc(child_type, NULL);
            if (!child) { free(child_name); return MC_ERR_OUT_OF_MEMORY; }
            child->name = child_name;

            err = nbt_read_payload(r, child_type, child, depth + 1);
            if (err) { nbt_free(child); return err; }

            err = nbt_compound_add(tag, child);
            if (err) { nbt_free(child); return err; }
        }
        return MC_OK;
    }
    case NBT_TAG_INT_ARRAY: {
        int32_t len;
        err = read_i32(r, &len);
        if (err) return err;
        if (len < 0 || len > NBT_MAX_ARRAY_LEN) return MC_ERR_IO;
        tag->payload.v_int_array.length = len;
        if (len > 0) {
            tag->payload.v_int_array.data = malloc((size_t)len * sizeof(int32_t));
            if (!tag->payload.v_int_array.data) return MC_ERR_OUT_OF_MEMORY;
            for (int32_t i = 0; i < len; i++) {
                err = read_i32(r, &tag->payload.v_int_array.data[i]);
                if (err) return err;
            }
        }
        return MC_OK;
    }
    default:
        return MC_ERR_IO;
    }
}

mc_error_t nbt_read(nbt_reader_t* reader, nbt_tag_t** out)
{
    if (!reader || !out) return MC_ERR_INVALID_ARG;
    return nbt_read_tag(reader, out, 0);
}

/* ------------------------------------------------------------------ */
/*  Binary write (recursive)                                           */
/* ------------------------------------------------------------------ */

static mc_error_t nbt_write_payload(nbt_writer_t* w, const nbt_tag_t* tag);

static mc_error_t nbt_write_tag_header(nbt_writer_t* w, const nbt_tag_t* tag)
{
    mc_error_t err = write_u8(w, tag->type);
    if (err) return err;

    const char* name = tag->name ? tag->name : "";
    uint16_t name_len = (uint16_t)strlen(name);
    return write_string_raw(w, name, name_len);
}

static mc_error_t nbt_write_payload(nbt_writer_t* w, const nbt_tag_t* tag)
{
    mc_error_t err;

    switch (tag->type) {
    case NBT_TAG_BYTE:
        return write_u8(w, (uint8_t)tag->payload.v_byte);

    case NBT_TAG_SHORT:
        return write_i16(w, tag->payload.v_short);

    case NBT_TAG_INT:
        return write_i32(w, tag->payload.v_int);

    case NBT_TAG_LONG:
        return write_i64(w, tag->payload.v_long);

    case NBT_TAG_FLOAT:
        return write_f32(w, tag->payload.v_float);

    case NBT_TAG_DOUBLE:
        return write_f64(w, tag->payload.v_double);

    case NBT_TAG_BYTE_ARRAY:
        err = write_i32(w, tag->payload.v_byte_array.length);
        if (err) return err;
        if (tag->payload.v_byte_array.length > 0)
            return writer_write(w, tag->payload.v_byte_array.data,
                                (size_t)tag->payload.v_byte_array.length);
        return MC_OK;

    case NBT_TAG_STRING:
        return write_string_raw(w, tag->payload.v_string.data,
                                tag->payload.v_string.length);

    case NBT_TAG_LIST: {
        err = write_u8(w, tag->payload.v_list.element_type);
        if (err) return err;
        err = write_i32(w, tag->payload.v_list.count);
        if (err) return err;
        for (int32_t i = 0; i < tag->payload.v_list.count; i++) {
            err = nbt_write_payload(w, tag->payload.v_list.items[i]);
            if (err) return err;
        }
        return MC_OK;
    }
    case NBT_TAG_COMPOUND: {
        for (int32_t i = 0; i < tag->payload.v_compound.count; i++) {
            const nbt_tag_t* child = tag->payload.v_compound.children[i];
            err = write_u8(w, child->type);
            if (err) return err;
            const char* cname = child->name ? child->name : "";
            err = write_string_raw(w, cname, (uint16_t)strlen(cname));
            if (err) return err;
            err = nbt_write_payload(w, child);
            if (err) return err;
        }
        return write_u8(w, NBT_TAG_END);
    }
    case NBT_TAG_INT_ARRAY: {
        err = write_i32(w, tag->payload.v_int_array.length);
        if (err) return err;
        for (int32_t i = 0; i < tag->payload.v_int_array.length; i++) {
            err = write_i32(w, tag->payload.v_int_array.data[i]);
            if (err) return err;
        }
        return MC_OK;
    }
    default:
        return MC_ERR_IO;
    }
}

mc_error_t nbt_write(nbt_writer_t* writer, const nbt_tag_t* tag)
{
    if (!writer || !tag) return MC_ERR_INVALID_ARG;

    mc_error_t err = nbt_write_tag_header(writer, tag);
    if (err) return err;
    return nbt_write_payload(writer, tag);
}

/* ------------------------------------------------------------------ */
/*  Writer init / free                                                 */
/* ------------------------------------------------------------------ */

void nbt_writer_init(nbt_writer_t* w)
{
    w->data = NULL;
    w->size = 0;
    w->capacity = 0;
}

void nbt_writer_free(nbt_writer_t* w)
{
    free(w->data);
    w->data = NULL;
    w->size = 0;
    w->capacity = 0;
}

/* ------------------------------------------------------------------ */
/*  File I/O                                                           */
/* ------------------------------------------------------------------ */

mc_error_t nbt_read_file(const char* path, nbt_tag_t** out)
{
    if (!path || !out) return MC_ERR_INVALID_ARG;

    FILE* f = fopen(path, "rb");
    if (!f) return MC_ERR_IO;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0) { fclose(f); return MC_ERR_IO; }

    uint8_t* buf = malloc((size_t)fsize);
    if (!buf) { fclose(f); return MC_ERR_OUT_OF_MEMORY; }

    if (fread(buf, 1, (size_t)fsize, f) != (size_t)fsize) {
        free(buf);
        fclose(f);
        return MC_ERR_IO;
    }
    fclose(f);

    nbt_reader_t reader = { .data = buf, .size = (size_t)fsize, .pos = 0 };
    mc_error_t err = nbt_read(&reader, out);
    free(buf);
    return err;
}

mc_error_t nbt_write_file(const char* path, const nbt_tag_t* tag)
{
    if (!path || !tag) return MC_ERR_INVALID_ARG;

    nbt_writer_t writer;
    nbt_writer_init(&writer);

    mc_error_t err = nbt_write(&writer, tag);
    if (err) { nbt_writer_free(&writer); return err; }

    FILE* f = fopen(path, "wb");
    if (!f) { nbt_writer_free(&writer); return MC_ERR_IO; }

    size_t total = writer.size;
    size_t written = fwrite(writer.data, 1, total, f);
    fclose(f);
    nbt_writer_free(&writer);

    return (written == total) ? MC_OK : MC_ERR_IO;
}
