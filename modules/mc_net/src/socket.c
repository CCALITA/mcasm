/* socket.c — UDP socket management for mc_net */

#include "mc_net_internal.h"
#include <stdio.h>
#include <sys/time.h>

/* Module-global state */
net_socket_state_t g_net_state;
handler_entry_t    g_handlers[NET_MAX_HANDLERS];
pending_packet_t   g_pending[NET_MAX_PENDING_ACKS];

/* ---------- time helper ---------- */

uint64_t net_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

/* ---------- helpers ---------- */

static void reset_all_state(void) {
    memset(&g_net_state, 0, sizeof(g_net_state));
    memset(g_handlers, 0, sizeof(g_handlers));
    memset(g_pending, 0, sizeof(g_pending));
    g_net_state.sock_fd = -1;
}

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* ---------- public API ---------- */

mc_error_t mc_net_init(void) {
    reset_all_state();
    g_net_state.next_seq = 1;
    return MC_OK;
}

void mc_net_shutdown(void) {
    mc_net_disconnect();
    reset_all_state();
}

mc_error_t mc_net_host(uint16_t port, uint32_t max_clients) {
    (void)max_clients; /* reserved for future connection table */

    if (g_net_state.sock_fd >= 0) {
        return MC_ERR_ALREADY_EXISTS;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return MC_ERR_NETWORK;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return MC_ERR_NETWORK;
    }

    if (set_nonblocking(fd) < 0) {
        close(fd);
        return MC_ERR_NETWORK;
    }

    g_net_state.sock_fd = fd;
    g_net_state.is_server = 1;
    g_net_state.connected = 1;
    return MC_OK;
}

mc_error_t mc_net_connect(const char* host, uint16_t port) {
    if (g_net_state.sock_fd >= 0) {
        return MC_ERR_ALREADY_EXISTS;
    }
    if (!host) {
        return MC_ERR_INVALID_ARG;
    }

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%u", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0 || !res) {
        return MC_ERR_NETWORK;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(res);
        return MC_ERR_NETWORK;
    }

    /* Store peer address for sendto */
    memcpy(&g_net_state.peer_addr, res->ai_addr, res->ai_addrlen);
    g_net_state.peer_addr_len = res->ai_addrlen;
    freeaddrinfo(res);

    if (set_nonblocking(fd) < 0) {
        close(fd);
        return MC_ERR_NETWORK;
    }

    g_net_state.sock_fd = fd;
    g_net_state.is_server = 0;
    g_net_state.connected = 1;
    return MC_OK;
}

void mc_net_disconnect(void) {
    if (g_net_state.sock_fd >= 0) {
        close(g_net_state.sock_fd);
        g_net_state.sock_fd = -1;
    }
    g_net_state.connected = 0;
    g_net_state.is_server = 0;
    g_net_state.peer_addr_len = 0;
    /* Clear pending packets */
    memset(g_pending, 0, sizeof(g_pending));
}

uint8_t mc_net_is_connected(void) {
    return g_net_state.connected;
}

/* Test helper: point peer address at the server's own loopback address.
 * Not part of the public API (not in mc_net.h). */
void mc_net_test_set_loopback_peer(uint16_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    memcpy(&g_net_state.peer_addr, &addr, sizeof(addr));
    g_net_state.peer_addr_len = sizeof(addr);
}
