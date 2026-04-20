#ifndef MC_NET_INTERNAL_H
#define MC_NET_INTERNAL_H

#include "mc_net.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

/* ---------- packet wire format ----------
 * [length:u16][packet_id:u8][sequence:u32][payload...]
 * length covers the entire frame (header + payload).
 */

#define NET_HEADER_SIZE       7   /* u16 + u8 + u32 */
#define NET_MAX_PACKET_SIZE   4096
#define NET_MAX_HANDLERS      256
#define NET_MAX_PENDING_ACKS  256
#define NET_RESEND_TIMEOUT_MS 500
#define NET_MAX_RESENDS       5

/* ---------- connection state ---------- */

typedef struct {
    int                      sock_fd;
    struct sockaddr_storage  peer_addr;
    socklen_t                peer_addr_len;
    uint8_t                  connected;
    uint8_t                  is_server;
    uint32_t                 next_seq;
} net_socket_state_t;

/* ---------- reliability ---------- */

typedef struct {
    uint32_t sequence;
    uint8_t  packet_id;
    uint8_t  data[NET_MAX_PACKET_SIZE];
    uint32_t data_size;
    uint64_t send_time_ms;
    uint8_t  resend_count;
    uint8_t  active;
} pending_packet_t;

/* ---------- handler registry ---------- */

typedef struct {
    mc_net_packet_handler_fn fn;
    void*                    userdata;
} handler_entry_t;

/* ---------- globals (module-private) ---------- */

extern net_socket_state_t g_net_state;
extern handler_entry_t    g_handlers[NET_MAX_HANDLERS];
extern pending_packet_t   g_pending[NET_MAX_PENDING_ACKS];

/* ---------- serialization (serialize.c) ---------- */

void     net_write_u16(uint8_t* buf, uint16_t val);
uint16_t net_read_u16(const uint8_t* buf);
void     net_write_u32(uint8_t* buf, uint32_t val);
uint32_t net_read_u32(const uint8_t* buf);
void     net_write_float(uint8_t* buf, float val);
float    net_read_float(const uint8_t* buf);
void     net_write_vec3(uint8_t* buf, const vec3_t* v);
void     net_read_vec3(const uint8_t* buf, vec3_t* v);
void     net_write_block_pos(uint8_t* buf, const block_pos_t* p);
void     net_read_block_pos(const uint8_t* buf, block_pos_t* p);

/* ---------- time helper ---------- */

uint64_t net_time_ms(void);

/* ---------- reliability (protocol.c) ---------- */

void mc_net_resend_pending(void);

#endif /* MC_NET_INTERNAL_H */
