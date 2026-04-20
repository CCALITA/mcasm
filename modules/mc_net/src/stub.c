#include "mc_net.h"
mc_error_t mc_net_init(void) { return MC_OK; }
void mc_net_shutdown(void) {}
mc_error_t mc_net_host(uint16_t port, uint32_t max) { (void)port;(void)max; return MC_OK; }
mc_error_t mc_net_connect(const char* h, uint16_t p) { (void)h;(void)p; return MC_OK; }
void mc_net_disconnect(void) {}
uint8_t mc_net_is_connected(void) { return 0; }
mc_error_t mc_net_send(uint8_t id, const void* d, uint32_t s) { (void)id;(void)d;(void)s; return MC_OK; }
uint32_t mc_net_receive(uint8_t* id, void* d, uint32_t max) { (void)id;(void)d;(void)max; return 0; }
void mc_net_tick(void) {}
void mc_net_set_handler(uint8_t id, mc_net_packet_handler_fn h, void* ud) { (void)id;(void)h;(void)ud; }
