/* serialize.c — Fast binary serialization in network byte order */

#include "mc_net_internal.h"
#include <string.h>

/* ---- 16-bit ---- */

void net_write_u16(uint8_t* buf, uint16_t val) {
    buf[0] = (uint8_t)(val >> 8);
    buf[1] = (uint8_t)(val);
}

uint16_t net_read_u16(const uint8_t* buf) {
    return (uint16_t)((uint16_t)buf[0] << 8 | (uint16_t)buf[1]);
}

/* ---- 32-bit ---- */

void net_write_u32(uint8_t* buf, uint32_t val) {
    buf[0] = (uint8_t)(val >> 24);
    buf[1] = (uint8_t)(val >> 16);
    buf[2] = (uint8_t)(val >> 8);
    buf[3] = (uint8_t)(val);
}

uint32_t net_read_u32(const uint8_t* buf) {
    return (uint32_t)buf[0] << 24
         | (uint32_t)buf[1] << 16
         | (uint32_t)buf[2] << 8
         | (uint32_t)buf[3];
}

/* ---- float (IEEE 754, transmitted as u32) ---- */

void net_write_float(uint8_t* buf, float val) {
    uint32_t bits;
    memcpy(&bits, &val, sizeof(bits));
    net_write_u32(buf, bits);
}

float net_read_float(const uint8_t* buf) {
    uint32_t bits = net_read_u32(buf);
    float val;
    memcpy(&val, &bits, sizeof(val));
    return val;
}

/* ---- vec3_t (3 floats, _pad excluded) ---- */

void net_write_vec3(uint8_t* buf, const vec3_t* v) {
    net_write_float(buf,     v->x);
    net_write_float(buf + 4, v->y);
    net_write_float(buf + 8, v->z);
}

void net_read_vec3(const uint8_t* buf, vec3_t* v) {
    v->x = net_read_float(buf);
    v->y = net_read_float(buf + 4);
    v->z = net_read_float(buf + 8);
    v->_pad = 0.0f;
}

/* ---- block_pos_t (3 x int32, _pad excluded) ---- */

void net_write_block_pos(uint8_t* buf, const block_pos_t* p) {
    net_write_u32(buf,     (uint32_t)p->x);
    net_write_u32(buf + 4, (uint32_t)p->y);
    net_write_u32(buf + 8, (uint32_t)p->z);
}

void net_read_block_pos(const uint8_t* buf, block_pos_t* p) {
    p->x = (int32_t)net_read_u32(buf);
    p->y = (int32_t)net_read_u32(buf + 4);
    p->z = (int32_t)net_read_u32(buf + 8);
    p->_pad = 0;
}
