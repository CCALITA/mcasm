#ifndef MC_NET_H
#define MC_NET_H

#include "mc_types.h"
#include "mc_error.h"

#define PACKET_PLAYER_POS    0x01
#define PACKET_BLOCK_CHANGE  0x02
#define PACKET_CHUNK_DATA    0x03
#define PACKET_ENTITY_SPAWN  0x04
#define PACKET_ENTITY_MOVE   0x05
#define PACKET_BLOCK_DIG     0x06
#define PACKET_BLOCK_PLACE   0x07
#define PACKET_KEEP_ALIVE    0x08
#define PACKET_CHAT_MESSAGE  0x0A

mc_error_t mc_net_init(void);
void       mc_net_shutdown(void);

mc_error_t mc_net_host(uint16_t port, uint32_t max_clients);
mc_error_t mc_net_connect(const char* host, uint16_t port);
void       mc_net_disconnect(void);
uint8_t    mc_net_is_connected(void);

mc_error_t mc_net_send(uint8_t packet_id, const void* data, uint32_t size);
uint32_t   mc_net_receive(uint8_t* out_packet_id, void* out_data, uint32_t max_size);

void       mc_net_tick(void);

typedef void (*mc_net_packet_handler_fn)(uint8_t packet_id, const void* data, uint32_t size, void* userdata);
void       mc_net_set_handler(uint8_t packet_id, mc_net_packet_handler_fn handler, void* userdata);

#endif /* MC_NET_H */
