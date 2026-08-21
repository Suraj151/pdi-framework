/****************************** Socket Tests **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Real loopback sockets, driven through the interfaces the framework consumes.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <interface/pdi.h>
#include <pditest.h>
#include <unistd.h>

static const ipaddress_t LOOPBACK(127, 0, 0, 1);

/**
 * connect() takes a NUL terminated host string, not four address bytes, so a
 * name resolves the same way here as it does on a board.
 */
static const uint8_t *LOOPBACK_HOST = (const uint8_t *)"127.0.0.1";

/**
 * Let the peer's data cross the loopback before it is read back.
 */
static void settle()
{
    usleep(20000);
}

/**
 * Stand up a listener on an ephemeral port and connect a client to it,
 * returning the server side of the pair through accepted.
 */
static bool connectedPair(TcpServerInterface &server, TcpClientInterface &client,
                          iClientInterface **accepted)
{
    if (0 != server.begin(0))
    {
        return false;
    }

    if (0 != client.connect(LOOPBACK_HOST, server.getBoundPort()))
    {
        return false;
    }

    for (int attempt = 0; attempt < 50; attempt++)
    {
        *accepted = server.accept();
        if (nullptr != *accepted)
        {
            return true;
        }
        usleep(2000);
    }

    return false;
}

TEST(tcp, a_server_binds_an_ephemeral_port)
{
    TcpServerInterface server;
    ASSERT_EQ(server.begin(0), 0);
    ASSERT_GT(server.getBoundPort(), (uint16_t)0);
    server.close();
}

TEST(tcp, a_fresh_server_has_no_client)
{
    TcpServerInterface server;
    server.begin(0);
    ASSERT_FALSE(server.hasClient());
    ASSERT_NULL(server.accept());
    server.close();
}

TEST(tcp, a_client_connects_and_is_accepted)
{
    TcpServerInterface server;
    TcpClientInterface client;
    iClientInterface *accepted = nullptr;

    ASSERT_TRUE(connectedPair(server, client, &accepted));
    ASSERT_NOT_NULL(accepted);
    ASSERT_EQ(client.connected(), (int8_t)1);

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, a_connection_to_a_closed_port_fails)
{
    TcpServerInterface server;
    server.begin(0);
    uint16_t port = server.getBoundPort();
    server.close();

    TcpClientInterface client;
    ASSERT_LT(client.connect(LOOPBACK_HOST, port), (int16_t)0);
}

TEST(tcp, a_client_connects_by_name)
{
    TcpServerInterface server;
    ASSERT_EQ(server.begin(0), 0);

    TcpClientInterface client;
    ASSERT_EQ(client.connect((const uint8_t *)"localhost", server.getBoundPort()), (int16_t)0);

    iClientInterface *accepted = nullptr;
    for (int attempt = 0; attempt < 50 && nullptr == accepted; attempt++)
    {
        accepted = server.accept();
        usleep(2000);
    }

    ASSERT_NOT_NULL(accepted);

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, an_unresolvable_host_fails)
{
    TcpClientInterface client;
    ASSERT_LT(client.connect((const uint8_t *)"no.such.host.invalid", (uint16_t)80), (int16_t)0);
}

TEST(tcp, data_travels_from_client_to_server)
{
    TcpServerInterface server;
    TcpClientInterface client;
    iClientInterface *accepted = nullptr;
    ASSERT_TRUE(connectedPair(server, client, &accepted));

    const char *message = "hello over tcp";
    ASSERT_EQ(client.write(message), (int32_t)strlen(message));
    settle();

    uint8_t buf[64];
    memset(buf, 0, sizeof(buf));
    ASSERT_EQ(accepted->read(buf, (uint32_t)strlen(message)), (int32_t)strlen(message));
    ASSERT_STREQ((const char *)buf, message);

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, data_travels_from_server_to_client)
{
    TcpServerInterface server;
    TcpClientInterface client;
    iClientInterface *accepted = nullptr;
    ASSERT_TRUE(connectedPair(server, client, &accepted));

    const char *message = "hello back";
    accepted->write(message);
    settle();

    uint8_t buf[64];
    memset(buf, 0, sizeof(buf));
    ASSERT_EQ(client.read(buf, (uint32_t)strlen(message)), (int32_t)strlen(message));
    ASSERT_STREQ((const char *)buf, message);

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, available_reports_pending_bytes)
{
    TcpServerInterface server;
    TcpClientInterface client;
    iClientInterface *accepted = nullptr;
    ASSERT_TRUE(connectedPair(server, client, &accepted));

    ASSERT_EQ(accepted->available(), 0);
    client.write("1234");
    settle();
    ASSERT_EQ(accepted->available(), 4);

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, byte_reads_keep_their_order)
{
    TcpServerInterface server;
    TcpClientInterface client;
    iClientInterface *accepted = nullptr;
    ASSERT_TRUE(connectedPair(server, client, &accepted));

    client.write("abc");
    settle();

    ASSERT_EQ(accepted->read(), (uint8_t)'a');
    ASSERT_EQ(accepted->read(), (uint8_t)'b');
    ASSERT_EQ(accepted->read(), (uint8_t)'c');

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, binary_payloads_survive_the_round_trip)
{
    TcpServerInterface server;
    TcpClientInterface client;
    iClientInterface *accepted = nullptr;
    ASSERT_TRUE(connectedPair(server, client, &accepted));

    const uint8_t payload[] = {0x00, 0xFF, 0x0D, 0x0A, 0x1A, 0x7F, 0x80};
    client.write(payload, sizeof(payload));
    settle();

    uint8_t buf[16];
    memset(buf, 0, sizeof(buf));
    ASSERT_EQ(accepted->read(buf, sizeof(payload)), (int32_t)sizeof(payload));
    ASSERT_MEMEQ(buf, payload, sizeof(payload));

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, read_string_until_works_across_the_link)
{
    TcpServerInterface server;
    TcpClientInterface client;
    iClientInterface *accepted = nullptr;
    ASSERT_TRUE(connectedPair(server, client, &accepted));

    client.write("GET /index.html HTTP/1.1\r\n");
    settle();

    pdiutil::string line;
    accepted->readLine(line);
    ASSERT_STREQ(line.c_str(), "GET /index.html HTTP/1.1");

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, a_closed_peer_shows_as_disconnected)
{
    TcpServerInterface server;
    TcpClientInterface client;
    iClientInterface *accepted = nullptr;
    ASSERT_TRUE(connectedPair(server, client, &accepted));

    client.close();
    settle();

    ASSERT_EQ(accepted->connected(), (int8_t)0);

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, the_accept_callback_fires)
{
    static int accepts = 0;
    accepts = 0;

    TcpServerInterface server;
    server.setOnAcceptClientEventCallback([](void *arg) { accepts++; });
    TcpClientInterface client;
    iClientInterface *accepted = nullptr;

    ASSERT_TRUE(connectedPair(server, client, &accepted));
    ASSERT_EQ(accepts, 1);

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, endpoints_report_their_addresses)
{
    TcpServerInterface server;
    TcpClientInterface client;
    iClientInterface *accepted = nullptr;
    ASSERT_TRUE(connectedPair(server, client, &accepted));

    ASSERT_EQ(client.getRemotePort(), server.getBoundPort());
    ASSERT_EQ(client.getRemoteIp().ip4[0], (uint8_t)127);
    ASSERT_GT(client.getLocalPort(), (uint16_t)0);

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, socket_options_are_accepted)
{
    TcpServerInterface server;
    TcpClientInterface client;
    iClientInterface *accepted = nullptr;
    ASSERT_TRUE(connectedPair(server, client, &accepted));

    ASSERT_TRUE(client.setKeepAlive(10, 5, 3));
    client.setNoDelay(true);
    ASSERT_TRUE(client.availableforwrite(1));

    pdiutil::safe_delete(accepted);
    server.close();
}

TEST(tcp, the_instance_factory_hands_out_sockets)
{
    iTcpServerInterface *server = __i_instance.getNewTcpServerInstance();
    iTcpClientInterface *clientside = __i_instance.getNewTcpClientInstance();

    ASSERT_NOT_NULL(server);
    ASSERT_NOT_NULL(clientside);

    pdiutil::safe_delete(server);
    pdiutil::safe_delete(clientside);
}

static int s_udp_packets = 0;
static uint8_t s_udp_payload[64];
static uint16_t s_udp_len = 0;

static void onUdpPacket(void *arg)
{
    udp_packet_t *packet = (udp_packet_t *)arg;
    s_udp_packets++;
    s_udp_len = packet->m_len;
    if (s_udp_len <= sizeof(s_udp_payload))
    {
        memcpy(s_udp_payload, packet->m_data, s_udp_len);
    }
}

TEST(udp, binds_an_ephemeral_port)
{
    UdpInterface udp;
    ASSERT_TRUE(udp.begin(0));
    ASSERT_GT(udp.getBoundPort(), (uint16_t)0);
    udp.close();
}

TEST(udp, a_datagram_reaches_the_callback)
{
    UdpInterface receiver;
    UdpInterface sender;

    s_udp_packets = 0;
    s_udp_len = 0;

    ASSERT_TRUE(receiver.begin(0));
    receiver.setOnPacketCallback(onUdpPacket);
    ASSERT_TRUE(sender.begin(0));

    const char *message = "udp payload";
    ASSERT_GT(sender.send((const uint8_t *)message, (uint16_t)strlen(message), LOOPBACK,
                          receiver.getBoundPort()),
              0);
    settle();

    ASSERT_EQ(receiver.service(), (uint16_t)1);
    ASSERT_EQ(s_udp_packets, 1);
    ASSERT_EQ(s_udp_len, (uint16_t)strlen(message));
    ASSERT_MEMEQ(s_udp_payload, message, s_udp_len);

    receiver.close();
    sender.close();
}

TEST(udp, the_sender_address_is_reported)
{
    static ipaddress_t seen;
    static uint16_t seenport = 0;
    seenport = 0;

    UdpInterface receiver;
    UdpInterface sender;

    ASSERT_TRUE(receiver.begin(0));
    receiver.setOnPacketCallback([](void *arg) {
        udp_packet_t *packet = (udp_packet_t *)arg;
        seen = packet->m_src_ip;
        seenport = packet->m_src_port;
    });
    ASSERT_TRUE(sender.begin(0));

    sender.send((const uint8_t *)"x", 1, LOOPBACK, receiver.getBoundPort());
    settle();
    receiver.service();

    ASSERT_EQ(seen.ip4[0], (uint8_t)127);
    ASSERT_EQ(seenport, sender.getBoundPort());

    receiver.close();
    sender.close();
}

TEST(udp, several_datagrams_are_all_delivered)
{
    UdpInterface receiver;
    UdpInterface sender;

    s_udp_packets = 0;

    ASSERT_TRUE(receiver.begin(0));
    receiver.setOnPacketCallback(onUdpPacket);
    ASSERT_TRUE(sender.begin(0));

    for (int i = 0; i < 3; i++)
    {
        sender.send((const uint8_t *)"pkt", 3, LOOPBACK, receiver.getBoundPort());
    }
    settle();

    ASSERT_EQ(receiver.service(), (uint16_t)3);
    ASSERT_EQ(s_udp_packets, 3);

    receiver.close();
    sender.close();
}

TEST(udp, service_on_an_idle_socket_delivers_nothing)
{
    UdpInterface receiver;
    ASSERT_TRUE(receiver.begin(0));
    receiver.setOnPacketCallback(onUdpPacket);

    ASSERT_EQ(receiver.service(), (uint16_t)0);
    receiver.close();
}

TEST(udp, sending_before_bind_is_refused)
{
    UdpInterface udp;
    ASSERT_LT(udp.send((const uint8_t *)"x", 1, LOOPBACK, 9999), 0);
}

TEST(udp, the_serve_loop_pump_drains_open_sockets)
{
    UdpInterface receiver;
    UdpInterface sender;

    s_udp_packets = 0;

    ASSERT_TRUE(receiver.begin(0));
    receiver.setOnPacketCallback(onUdpPacket);
    ASSERT_TRUE(sender.begin(0));

    sender.send((const uint8_t *)"pumped", 6, LOOPBACK, receiver.getBoundPort());
    settle();

    // this is what the device control interface calls out of handleEvents
    __i_dvc_ctrl.handleEvents();
    ASSERT_EQ(s_udp_packets, 1);

    receiver.close();
    sender.close();
}

TEST(udp, the_instance_factory_hands_out_sockets)
{
    iUdpInterface *udp = __i_instance.getNewUdpInstance();
    ASSERT_NOT_NULL(udp);
    pdiutil::safe_delete(udp);
}
