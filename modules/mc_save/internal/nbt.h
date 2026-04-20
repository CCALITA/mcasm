#ifndef MC_SAVE_NBT_H
#define MC_SAVE_NBT_H

#include "mc_error.h"
#include <stdint.h>
#include <stddef.h>

/* NBT tag type identifiers (matches Minecraft's NBT specification). */
#define NBT_TAG_END        0
#define NBT_TAG_BYTE       1
#define NBT_TAG_SHORT      2
#define NBT_TAG_INT        3
#define NBT_TAG_LONG       4
#define NBT_TAG_FLOAT      5
#define NBT_TAG_DOUBLE     6
#define NBT_TAG_BYTE_ARRAY 7
#define NBT_TAG_STRING     8
#define NBT_TAG_LIST       9
#define NBT_TAG_COMPOUND   10
#define NBT_TAG_INT_ARRAY  11

/* Maximum limits for safety. */
#define NBT_MAX_CHILDREN    512
#define NBT_MAX_ARRAY_LEN   (16 * 16 * 16 * 24)  /* enough for a full chunk */
#define NBT_MAX_STRING_LEN  32767
#define NBT_MAX_DEPTH       32

/* Forward declaration. */
typedef struct nbt_tag nbt_tag_t;

/* A single NBT tag. Compound/List tags own their children. */
struct nbt_tag {
    uint8_t      type;
    char*        name;       /* NULL for list elements and TAG_End */

    union {
        int8_t   v_byte;
        int16_t  v_short;
        int32_t  v_int;
        int64_t  v_long;
        float    v_float;
        double   v_double;

        struct {             /* TAG_Byte_Array */
            int8_t*  data;
            int32_t  length;
        } v_byte_array;

        struct {             /* TAG_String */
            char*    data;
            uint16_t length;
        } v_string;

        struct {             /* TAG_List */
            uint8_t     element_type;
            nbt_tag_t** items;
            int32_t     count;
            int32_t     capacity;
        } v_list;

        struct {             /* TAG_Compound */
            nbt_tag_t** children;
            int32_t     count;
            int32_t     capacity;
        } v_compound;

        struct {             /* TAG_Int_Array */
            int32_t* data;
            int32_t  length;
        } v_int_array;
    } payload;
};

/* ---------- Buffer-based I/O (used internally) ---------- */

typedef struct {
    const uint8_t* data;
    size_t         size;
    size_t         pos;
} nbt_reader_t;

typedef struct {
    uint8_t* data;
    size_t   size;
    size_t   capacity;
} nbt_writer_t;

/* ---------- Lifecycle ---------- */

nbt_tag_t*  nbt_create_byte(const char* name, int8_t value);
nbt_tag_t*  nbt_create_short(const char* name, int16_t value);
nbt_tag_t*  nbt_create_int(const char* name, int32_t value);
nbt_tag_t*  nbt_create_long(const char* name, int64_t value);
nbt_tag_t*  nbt_create_float(const char* name, float value);
nbt_tag_t*  nbt_create_double(const char* name, double value);
nbt_tag_t*  nbt_create_byte_array(const char* name, const int8_t* data, int32_t len);
nbt_tag_t*  nbt_create_string(const char* name, const char* value);
nbt_tag_t*  nbt_create_list(const char* name, uint8_t element_type);
nbt_tag_t*  nbt_create_compound(const char* name);
nbt_tag_t*  nbt_create_int_array(const char* name, const int32_t* data, int32_t len);

void        nbt_free(nbt_tag_t* tag);

/* ---------- Compound/List helpers ---------- */

mc_error_t  nbt_compound_add(nbt_tag_t* compound, nbt_tag_t* child);
nbt_tag_t*  nbt_compound_get(const nbt_tag_t* compound, const char* name);
mc_error_t  nbt_list_append(nbt_tag_t* list, nbt_tag_t* item);

/* ---------- Binary serialization (big-endian, Minecraft format) ---------- */

mc_error_t  nbt_read(nbt_reader_t* reader, nbt_tag_t** out);
mc_error_t  nbt_write(nbt_writer_t* writer, const nbt_tag_t* tag);

/* ---------- File I/O ---------- */

mc_error_t  nbt_read_file(const char* path, nbt_tag_t** out);
mc_error_t  nbt_write_file(const char* path, const nbt_tag_t* tag);

/* ---------- Writer helpers ---------- */

void        nbt_writer_init(nbt_writer_t* w);
void        nbt_writer_free(nbt_writer_t* w);

#endif /* MC_SAVE_NBT_H */
