/* handlers.c — Packet handler registry and tick dispatch */

#include "mc_net_internal.h"

void mc_net_set_handler(uint8_t packet_id,
                        mc_net_packet_handler_fn handler,
                        void* userdata) {
    g_handlers[packet_id].fn = handler;
    g_handlers[packet_id].userdata = userdata;
}

void mc_net_tick(void) {
    if (!g_net_state.connected) return;

    /* Poll for incoming packets and dispatch */
    uint8_t buf[NET_MAX_PACKET_SIZE];
    uint8_t packet_id = 0;

    for (int polls = 0; polls < 64; polls++) {
        uint32_t payload_size = mc_net_receive(&packet_id, buf, sizeof(buf));
        if (payload_size == 0) break;

        handler_entry_t h = g_handlers[packet_id];
        if (h.fn) {
            h.fn(packet_id, buf, payload_size, h.userdata);
        }
    }

    /* Reliability: resend unacknowledged packets */
    mc_net_resend_pending();
}
