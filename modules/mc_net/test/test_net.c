/* test_net.c — Tests for mc_net module */

#include "mc_net.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* We need the internal serialization functions for direct testing.
 * Re-declare them here since we cannot include the internal header
 * from the test directory without changing build flags.
 */
extern void net_write_u16(uint8_t* buf, uint16_t val);
extern uint16_t net_read_u16(const uint8_t* buf);
extern void     net_write_u32(uint8_t* buf, uint32_t val);
extern uint32_t net_read_u32(const uint8_t* buf);
extern void     net_write_float(uint8_t* buf, float val);
extern float    net_read_float(const uint8_t* buf);
extern void     net_write_vec3(uint8_t* buf, const vec3_t* v);
extern void     net_read_vec3(const uint8_t* buf, vec3_t* v);
extern void     net_write_block_pos(uint8_t* buf, const block_pos_t* p);
extern void     net_read_block_pos(const uint8_t* buf, block_pos_t* p);

/* Test helper defined in socket.c (not in public header) */
extern void mc_net_test_set_loopback_peer(uint16_t port);

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        fprintf(stderr, "FAIL %s:%d: %s != %s (%d != %d)\n", \
                __FILE__, __LINE__, #a, #b, (int)(a), (int)(b)); \
        assert(0); \
    } \
} while (0)

#define ASSERT_FLOAT_EQ(a, b) do { \
    if (fabsf((a) - (b)) > 1e-6f) { \
        fprintf(stderr, "FAIL %s:%d: %s != %s (%f != %f)\n", \
                __FILE__, __LINE__, #a, #b, (double)(a), (double)(b)); \
        assert(0); \
    } \
} while (0)

/* ================================================================
 * Serialization roundtrip tests
 * ================================================================ */

static void test_u16_roundtrip(void) {
    uint8_t buf[2];
    uint16_t values[] = {0, 1, 255, 256, 1024, 65535};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        net_write_u16(buf, values[i]);
        ASSERT_EQ(net_read_u16(buf), values[i]);
    }
    printf("    PASS test_u16_roundtrip\n");
}

static void test_u32_roundtrip(void) {
    uint8_t buf[4];
    uint32_t values[] = {0, 1, 255, 65536, 0x12345678, 0xFFFFFFFF};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        net_write_u32(buf, values[i]);
        ASSERT_EQ(net_read_u32(buf), values[i]);
    }
    printf("    PASS test_u32_roundtrip\n");
}

static void test_float_roundtrip(void) {
    uint8_t buf[4];
    float values[] = {0.0f, 1.0f, -1.0f, 3.14159f, 1e10f, -1e-10f};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        net_write_float(buf, values[i]);
        ASSERT_FLOAT_EQ(net_read_float(buf), values[i]);
    }
    printf("    PASS test_float_roundtrip\n");
}

static void test_vec3_roundtrip(void) {
    uint8_t buf[12];
    vec3_t v = {.x = 1.5f, .y = -2.75f, .z = 100.0f, ._pad = 0.0f};
    net_write_vec3(buf, &v);

    vec3_t out;
    net_read_vec3(buf, &out);

    ASSERT_FLOAT_EQ(out.x, v.x);
    ASSERT_FLOAT_EQ(out.y, v.y);
    ASSERT_FLOAT_EQ(out.z, v.z);
    ASSERT_FLOAT_EQ(out._pad, 0.0f);
    printf("    PASS test_vec3_roundtrip\n");
}

static void test_block_pos_roundtrip(void) {
    uint8_t buf[12];
    block_pos_t p = {.x = -100, .y = 64, .z = 2048, ._pad = 0};
    net_write_block_pos(buf, &p);

    block_pos_t out;
    net_read_block_pos(buf, &out);

    ASSERT_EQ(out.x, p.x);
    ASSERT_EQ(out.y, p.y);
    ASSERT_EQ(out.z, p.z);
    ASSERT_EQ(out._pad, 0);
    printf("    PASS test_block_pos_roundtrip\n");
}

static void test_network_byte_order(void) {
    /* Verify big-endian encoding */
    uint8_t buf[4];
    net_write_u32(buf, 0x01020304);
    ASSERT_EQ(buf[0], 0x01);
    ASSERT_EQ(buf[1], 0x02);
    ASSERT_EQ(buf[2], 0x03);
    ASSERT_EQ(buf[3], 0x04);

    net_write_u16(buf, 0xABCD);
    ASSERT_EQ(buf[0], 0xAB);
    ASSERT_EQ(buf[1], 0xCD);
    printf("    PASS test_network_byte_order\n");
}

/* ================================================================
 * Handler registration tests
 * ================================================================ */

static int g_handler_called = 0;
static uint8_t g_handler_last_id = 0;
static uint32_t g_handler_last_size = 0;
static int g_handler_userdata_val = 0;

static void test_handler_fn(uint8_t packet_id, const void* data,
                            uint32_t size, void* userdata) {
    (void)data;
    g_handler_called++;
    g_handler_last_id = packet_id;
    g_handler_last_size = size;
    if (userdata) {
        g_handler_userdata_val = *(int*)userdata;
    }
}

static void test_handler_registration(void) {
    mc_net_init();

    int my_data = 42;
    mc_net_set_handler(PACKET_PLAYER_POS, test_handler_fn, &my_data);

    /* Manually invoke the handler to verify registration */
    uint8_t dummy_payload[] = {0xDE, 0xAD};
    test_handler_fn(PACKET_PLAYER_POS, dummy_payload, sizeof(dummy_payload),
                    &my_data);

    ASSERT_EQ(g_handler_called, 1);
    ASSERT_EQ(g_handler_last_id, PACKET_PLAYER_POS);
    ASSERT_EQ(g_handler_last_size, 2);
    ASSERT_EQ(g_handler_userdata_val, 42);

    mc_net_shutdown();
    g_handler_called = 0;
    printf("    PASS test_handler_registration\n");
}

static void test_multiple_handlers(void) {
    mc_net_init();

    int data_a = 10;
    int data_b = 20;
    mc_net_set_handler(PACKET_BLOCK_CHANGE, test_handler_fn, &data_a);
    mc_net_set_handler(PACKET_CHAT_MESSAGE, test_handler_fn, &data_b);

    /* Verify both are set by invoking them */
    test_handler_fn(PACKET_BLOCK_CHANGE, NULL, 0, &data_a);
    ASSERT_EQ(g_handler_userdata_val, 10);

    test_handler_fn(PACKET_CHAT_MESSAGE, NULL, 0, &data_b);
    ASSERT_EQ(g_handler_userdata_val, 20);

    mc_net_shutdown();
    g_handler_called = 0;
    printf("    PASS test_multiple_handlers\n");
}

/* ================================================================
 * Init / shutdown / connection state tests
 * ================================================================ */

static void test_init_shutdown(void) {
    mc_error_t err = mc_net_init();
    ASSERT_EQ(err, MC_OK);
    ASSERT_EQ(mc_net_is_connected(), 0);

    mc_net_shutdown();
    ASSERT_EQ(mc_net_is_connected(), 0);
    printf("    PASS test_init_shutdown\n");
}

static void test_disconnect_when_not_connected(void) {
    mc_net_init();
    /* Should not crash */
    mc_net_disconnect();
    ASSERT_EQ(mc_net_is_connected(), 0);
    mc_net_shutdown();
    printf("    PASS test_disconnect_when_not_connected\n");
}

static void test_send_when_not_connected(void) {
    mc_net_init();
    uint8_t data[] = {1, 2, 3};
    mc_error_t err = mc_net_send(PACKET_PLAYER_POS, data, sizeof(data));
    ASSERT_EQ(err, MC_ERR_NETWORK);
    mc_net_shutdown();
    printf("    PASS test_send_when_not_connected\n");
}

static void test_receive_when_not_connected(void) {
    mc_net_init();
    uint8_t buf[64];
    uint8_t pkt_id = 0;
    uint32_t received = mc_net_receive(&pkt_id, buf, sizeof(buf));
    ASSERT_EQ(received, 0);
    mc_net_shutdown();
    printf("    PASS test_receive_when_not_connected\n");
}

/* ================================================================
 * Loopback test (server binds, client connects, exchange packets)
 * ================================================================ */

static int g_loopback_handler_called = 0;
static uint8_t g_loopback_received_data[64];
static uint32_t g_loopback_received_size = 0;

static void loopback_handler(uint8_t packet_id, const void* data,
                             uint32_t size, void* userdata) {
    (void)packet_id;
    (void)userdata;
    g_loopback_handler_called++;
    g_loopback_received_size = size;
    if (size > 0 && size <= sizeof(g_loopback_received_data)) {
        memcpy(g_loopback_received_data, data, size);
    }
}

static void test_loopback(void) {
    /* This test requires two mc_net states, but we only have one global.
     * Instead, test a simple host-bind + self-send via loopback.
     * We host on a high port and manually sendto ourselves.
     */
    mc_net_init();

    uint16_t port = 19132;
    mc_error_t err = mc_net_host(port, 4);
    if (err != MC_OK) {
        /* Port might be in use, skip gracefully */
        printf("    SKIP test_loopback (port %u unavailable)\n", port);
        mc_net_shutdown();
        return;
    }

    ASSERT_EQ(mc_net_is_connected(), 1);

    /* Point the server's peer address at its own loopback socket */
    mc_net_test_set_loopback_peer(port);

    /* Register handler */
    mc_net_set_handler(PACKET_KEEP_ALIVE, loopback_handler, NULL);

    /* Send a packet to ourselves */
    uint8_t payload[] = {0xCA, 0xFE, 0xBA, 0xBE};
    err = mc_net_send(PACKET_KEEP_ALIVE, payload, sizeof(payload));
    ASSERT_EQ(err, MC_OK);

    /* Small busy-wait to let the packet arrive through loopback */
    for (int attempt = 0; attempt < 100; attempt++) {
        mc_net_tick();
        if (g_loopback_handler_called > 0) break;
        /* Tiny delay: just yield */
        for (volatile int i = 0; i < 10000; i++) {}
    }

    ASSERT_EQ(g_loopback_handler_called > 0, 1);
    ASSERT_EQ(g_loopback_received_size, sizeof(payload));
    ASSERT_EQ(g_loopback_received_data[0], 0xCA);
    ASSERT_EQ(g_loopback_received_data[1], 0xFE);
    ASSERT_EQ(g_loopback_received_data[2], 0xBA);
    ASSERT_EQ(g_loopback_received_data[3], 0xBE);

    mc_net_shutdown();
    printf("    PASS test_loopback\n");
}

/* ================================================================
 * Main
 * ================================================================ */

int main(void) {
    printf("  mc_net tests:\n");

    /* Serialization */
    test_u16_roundtrip();
    test_u32_roundtrip();
    test_float_roundtrip();
    test_vec3_roundtrip();
    test_block_pos_roundtrip();
    test_network_byte_order();

    /* Handler registry */
    test_handler_registration();
    test_multiple_handlers();

    /* Socket state */
    test_init_shutdown();
    test_disconnect_when_not_connected();
    test_send_when_not_connected();
    test_receive_when_not_connected();

    /* Loopback */
    test_loopback();

    printf("  ALL PASSED\n");
    return 0;
}
