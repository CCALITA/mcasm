/* protocol.c — Packet serialization, sending, receiving, and reliability */

#include "mc_net_internal.h"

/* ---------- internal helpers ---------- */

static int find_free_pending_slot(void) {
    for (int i = 0; i < NET_MAX_PENDING_ACKS; i++) {
        if (!g_pending[i].active) return i;
    }
    return -1;
}

static void store_pending(uint32_t seq, uint8_t packet_id,
                          const uint8_t* frame, uint32_t frame_size) {
    int slot = find_free_pending_slot();
    if (slot < 0) return; /* drop if table full */

    pending_packet_t* p = &g_pending[slot];
    p->sequence     = seq;
    p->packet_id    = packet_id;
    p->data_size    = frame_size;
    p->send_time_ms = net_time_ms();
    p->resend_count = 0;
    p->active       = 1;
    memcpy(p->data, frame, frame_size);
}

static void handle_ack(uint32_t ack_seq) {
    for (int i = 0; i < NET_MAX_PENDING_ACKS; i++) {
        if (g_pending[i].active && g_pending[i].sequence == ack_seq) {
            g_pending[i].active = 0;
            return;
        }
    }
}

static mc_error_t raw_send(const uint8_t* buf, uint32_t len) {
    if (g_net_state.peer_addr_len == 0) {
        return MC_ERR_NETWORK;
    }

    ssize_t sent = sendto(g_net_state.sock_fd, buf, len, 0,
                          (struct sockaddr*)&g_net_state.peer_addr,
                          g_net_state.peer_addr_len);

    return (sent >= 0) ? MC_OK : MC_ERR_NETWORK;
}

/* ---------- public API ---------- */

mc_error_t mc_net_send(uint8_t packet_id, const void* data, uint32_t size) {
    if (!g_net_state.connected || g_net_state.sock_fd < 0) {
        return MC_ERR_NETWORK;
    }
    if (size + NET_HEADER_SIZE > NET_MAX_PACKET_SIZE) {
        return MC_ERR_INVALID_ARG;
    }

    uint8_t frame[NET_MAX_PACKET_SIZE];
    uint16_t total_len = (uint16_t)(NET_HEADER_SIZE + size);
    uint32_t seq = g_net_state.next_seq++;

    /* Build header */
    net_write_u16(frame, total_len);
    frame[2] = packet_id;
    net_write_u32(frame + 3, seq);

    /* Copy payload */
    if (size > 0 && data) {
        memcpy(frame + NET_HEADER_SIZE, data, size);
    }

    /* Store for reliability (all non-ACK packets) */
    store_pending(seq, packet_id, frame, total_len);

    return raw_send(frame, total_len);
}

uint32_t mc_net_receive(uint8_t* out_packet_id, void* out_data,
                        uint32_t max_size) {
    if (!g_net_state.connected || g_net_state.sock_fd < 0) {
        return 0;
    }

    uint8_t frame[NET_MAX_PACKET_SIZE];
    struct sockaddr_storage from_addr;
    socklen_t from_len = sizeof(from_addr);

    ssize_t n = recvfrom(g_net_state.sock_fd, frame, sizeof(frame), 0,
                         (struct sockaddr*)&from_addr, &from_len);

    if (n < (ssize_t)NET_HEADER_SIZE) {
        return 0; /* nothing received or runt packet */
    }

    /* Parse header */
    uint16_t total_len = net_read_u16(frame);
    uint8_t  pkt_id    = frame[2];
    uint32_t seq       = net_read_u32(frame + 3);

    /* Sanity check */
    if (total_len > (uint16_t)n || total_len < NET_HEADER_SIZE) {
        return 0;
    }

    uint32_t payload_size = total_len - NET_HEADER_SIZE;

    /* For server: remember last sender for reply */
    if (g_net_state.is_server) {
        memcpy(&g_net_state.peer_addr, &from_addr, from_len);
        g_net_state.peer_addr_len = from_len;
    }

    /* ACK tracking: clear any pending outbound packet with this sequence.
     * In a full protocol the peer would send explicit ACK packets;
     * for now we treat any received sequence as an implicit ACK of
     * the same sequence number we sent (works for echo / keepalive). */
    handle_ack(seq);

    /* Copy out */
    if (out_packet_id) *out_packet_id = pkt_id;
    if (out_data && payload_size > 0) {
        uint32_t copy_size = payload_size < max_size ? payload_size : max_size;
        memcpy(out_data, frame + NET_HEADER_SIZE, copy_size);
    }

    return payload_size;
}

/* ---------- reliability tick ---------- */

void mc_net_resend_pending(void) {
    uint64_t now = net_time_ms();

    for (int i = 0; i < NET_MAX_PENDING_ACKS; i++) {
        if (!g_pending[i].active) continue;

        uint64_t elapsed = now - g_pending[i].send_time_ms;
        if (elapsed < NET_RESEND_TIMEOUT_MS) continue;

        if (g_pending[i].resend_count >= NET_MAX_RESENDS) {
            g_pending[i].active = 0; /* give up */
            continue;
        }

        /* Resend the frame */
        raw_send(g_pending[i].data, g_pending[i].data_size);
        g_pending[i].send_time_ms = now;
        g_pending[i].resend_count++;
    }
}
