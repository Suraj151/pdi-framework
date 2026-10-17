/**************************** MQTT Client Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers what the client makes of the broker's answer to its CONNECT: only an
accepted CONNACK makes it connected, a refused or malformed one leaves it
failed and disconnected, waiting to try again.

Author          : Suraj I.
created Date    : 3rd Oct 2026
******************************************************************************/

#include <MountedStack.h>
#include <pditest.h>

#include <transports/mqtt/MqttClient.h>
#include <string>
#include <vector>

#ifdef ENABLE_MQTT_SERVICE

namespace {

/**
 * A connection that answers with scripted bytes, zeros included, and records
 * whether the client hung up.
 */
class ScriptedBroker : public iClientInterface
{

public:

    ScriptedBroker(const uint8_t *answer, uint32_t size) : m_limit(size), m_answer(answer), m_size(size), m_at(0), m_connected(0) {}

    int16_t connect(const uint8_t *host, uint16_t port) override { m_connected = 1; return 1; }
    int16_t disconnect() override { m_connected = 0; return 1; }
    int8_t connected() override { return m_connected; }
    void setTimeout(uint32_t timeout) override {}

    int32_t write(uint8_t c) override { m_written.push_back(c); return 1; }
    int32_t write(const uint8_t *c_str) override
    {
        uint32_t size = (uint32_t)strlen((const char *)c_str);
        m_written.insert(m_written.end(), c_str, c_str + size);
        return (int32_t)size;
    }
    int32_t write(const uint8_t *c_str, uint32_t size) override
    {
        m_written.insert(m_written.end(), c_str, c_str + size);
        return (int32_t)size;
    }

    /**
     * Whether the client wrote a two byte acknowledgement of this type and packet id.
     */
    bool sawWritten(uint8_t type, uint16_t packet_id) const
    {
        for (size_t i = 0; i + 3 < m_written.size(); i++)
        {
            if ((m_written[i] & 0xF0) == type && 0x02 == m_written[i + 1] &&
                (uint8_t)(packet_id >> 8) == m_written[i + 2] && (uint8_t)packet_id == m_written[i + 3])
            {
                return true;
            }
        }
        return false;
    }

    std::vector<uint8_t> m_written;

    /**
     * How much of the answer has arrived so far; the rest is held back.
     */
    uint32_t m_limit;

    int32_t available() override { return (int32_t)(m_limit > m_at ? m_limit - m_at : 0); }

    uint8_t read() override
    {
        if (m_at >= m_limit) return 0;
        return m_answer[m_at++];
    }

    int32_t read(uint8_t *buf, uint32_t size) override
    {
        uint32_t taken = 0;
        while (taken < size && m_at < m_limit) {
            buf[taken++] = m_answer[m_at++];
        }
        return (int32_t)taken;
    }

private:

    const uint8_t *m_answer;
    uint32_t m_size;
    uint32_t m_at;
    int8_t m_connected;
};

/**
 * Connect a client through the broker double and let it read the answer.
 */
void connectAndRead(MQTTClient &mqtt, ScriptedBroker &broker)
{
    mqtt_general_config_table general;
    mqtt_lwt_config_table lwt;
    strcpy(general.host, "broker.example");
    general.port = 1883;
    strcpy(general.client_id, "device@test");
    strcpy(general.username, "00000001-SENSOR01-R01-C01-2026-000001");
    strcpy(general.password, "device-key");
    general.keepalive = 10;

    mqtt.begin(&broker, &general, &lwt);
    mqtt.MQTT_Task();
}

} // namespace

TEST(mqttclient, a_connack_return_code_is_read)
{
    uint8_t accepted[] = {0x20, 0x02, 0x00, 0x00};
    uint8_t refused[] = {0x20, 0x02, 0x00, 0x05};

    ASSERT_EQ(mqtt_get_connect_return_code(accepted, sizeof(accepted)), (uint8_t)MQTT_CONNACK_ACCEPTED);
    ASSERT_EQ(mqtt_get_connect_return_code(refused, sizeof(refused)), (uint8_t)5);
}

TEST(mqttclient, a_connack_that_cannot_carry_a_code_is_malformed)
{
    uint8_t shortPacket[] = {0x20, 0x02, 0x00};
    uint8_t notConnack[] = {0x30, 0x02, 0x00, 0x00};
    uint8_t wrongLength[] = {0x20, 0x03, 0x00, 0x00};

    ASSERT_EQ(mqtt_get_connect_return_code(shortPacket, sizeof(shortPacket)), (uint8_t)MQTT_CONNACK_MALFORMED);
    ASSERT_EQ(mqtt_get_connect_return_code(notConnack, sizeof(notConnack)), (uint8_t)MQTT_CONNACK_MALFORMED);
    ASSERT_EQ(mqtt_get_connect_return_code(wrongLength, sizeof(wrongLength)), (uint8_t)MQTT_CONNACK_MALFORMED);
    ASSERT_EQ(mqtt_get_connect_return_code(nullptr, 4), (uint8_t)MQTT_CONNACK_MALFORMED);
}

TEST(mqttclient, a_client_built_over_dirty_memory_holds_nothing_to_free)
{
    // a global starts zeroed, so the client is built over garbage here on
    // purpose, the way a stack or heap instance is
    alignas(MQTTClient) unsigned char storage[sizeof(MQTTClient)];
    memset(storage, 0xA5, sizeof(storage));

    MQTTClient *mqtt = new (storage) MQTTClient();

    ASSERT_NULL(mqtt->m_mqttClient.connect_info.client_id);
    ASSERT_NULL(mqtt->m_mqttClient.connect_info.username);
    ASSERT_NULL(mqtt->m_mqttClient.connect_info.password);
    ASSERT_NULL(mqtt->m_mqttClient.connect_info.will_topic);
    ASSERT_NULL(mqtt->m_mqttClient.connect_info.will_message);
    ASSERT_NULL(mqtt->m_mqttClient.mqtt_state.in_buffer);
    ASSERT_NULL(mqtt->m_mqttClient.mqtt_state.out_buffer);

    static const uint8_t answer[] = {0x20, 0x02, 0x00, 0x00};
    ScriptedBroker broker(answer, sizeof(answer));
    connectAndRead(*mqtt, broker);
    ASSERT_TRUE(mqtt->is_mqtt_connected());

    mqtt->~MQTTClient();
}

TEST(mqttclient, an_accepted_connack_connects)
{
    static const uint8_t answer[] = {0x20, 0x02, 0x00, 0x00};
    ScriptedBroker broker(answer, sizeof(answer));
    MQTTClient mqtt;

    connectAndRead(mqtt, broker);

    ASSERT_EQ((int)mqtt.m_mqttClient.connState, (int)MQTT_DATA);
    ASSERT_TRUE(mqtt.is_mqtt_connected());
}

TEST(mqttclient, a_refused_connack_is_not_a_connection)
{
    static const uint8_t answer[] = {0x20, 0x02, 0x00, 0x05};
    ScriptedBroker broker(answer, sizeof(answer));
    MQTTClient mqtt;

    connectAndRead(mqtt, broker);

    ASSERT_EQ((int)mqtt.m_mqttClient.connState, (int)MQTT_CONNECT_FAILED);
    ASSERT_FALSE(mqtt.m_mqttClient.mqtt_connected);
    ASSERT_FALSE(mqtt.is_mqtt_connected());
    ASSERT_EQ((int)broker.connected(), 0);
}

TEST(mqttclient, configuring_again_keeps_the_client_usable)
{
    static const uint8_t answer[] = {0x20, 0x02, 0x00, 0x00};
    ScriptedBroker first(answer, sizeof(answer));
    ScriptedBroker second(answer, sizeof(answer));
    MQTTClient mqtt;

    connectAndRead(mqtt, first);
    connectAndRead(mqtt, second);

    ASSERT_EQ((int)mqtt.m_mqttClient.connState, (int)MQTT_DATA);
    ASSERT_TRUE(mqtt.is_mqtt_connected());
    ASSERT_EQ(mqtt.m_mqttClient.subscribed_topics.size(), (size_t)0);
}

namespace {

uint32_t g_delivered = 0;
std::string g_last_data;

void countDelivery(uint32_t *args, const char *topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
    g_delivered++;
    g_last_data.assign(data, data_len);
}

} // namespace

TEST(mqttclient, every_publish_from_the_broker_is_delivered_in_order)
{
    static const uint8_t answer[] = {
        0x20, 0x02, 0x00, 0x00,
        0x30, 0x06, 0x00, 0x01, 't', 'o', 'n', 'e',
        0x30, 0x06, 0x00, 0x01, 't', 't', 'w', 'o',
        0x30, 0x08, 0x00, 0x01, 't', 't', 'h', 'r', 'e', 'e',
    };
    ScriptedBroker broker(answer, sizeof(answer));
    MQTTClient mqtt;
    g_delivered = 0;
    mqtt.OnData(countDelivery);

    connectAndRead(mqtt, broker);
    for (int i = 0; i < 10 && g_delivered < 3; i++)
    {
        mqtt.MQTT_Task();
    }

    ASSERT_EQ(g_delivered, 3u);
    ASSERT_STREQ(g_last_data.c_str(), "three");
}

TEST(mqttclient, a_burst_from_the_broker_is_read_in_one_task_not_one_per_tick)
{
    static const uint8_t answer[] = {
        0x20, 0x02, 0x00, 0x00,
        0x90, 0x03, 0x00, 0x01, 0x00,
        0x30, 0x06, 0x00, 0x01, 't', 'o', 'n', 'e',
        0x30, 0x06, 0x00, 0x01, 't', 't', 'w', 'o',
        0x30, 0x08, 0x00, 0x01, 't', 't', 'h', 'r', 'e', 'e',
    };
    ScriptedBroker broker(answer, sizeof(answer));
    MQTTClient mqtt;
    g_delivered = 0;
    mqtt.OnData(countDelivery);

    connectAndRead(mqtt, broker);
    mqtt.MQTT_Task();

    ASSERT_EQ(g_delivered, 3u);
    ASSERT_EQ(broker.available(), 0);
}

namespace {

/**
 * Connects, then hands the client the given bytes from the broker followed by
 * one good publish, and reports whether that good one was delivered.
 */
bool deliveredAfter(const uint8_t *garbage, uint32_t size)
{
    static uint8_t answer[2048];
    static const uint8_t connack[] = {0x20, 0x02, 0x00, 0x00};
    static const uint8_t good[] = {0x30, 0x07, 0x00, 0x01, 't', 'g', 'o', 'o', 'd'};

    uint32_t at = 0;
    memcpy(answer + at, connack, sizeof(connack)); at += sizeof(connack);
    memcpy(answer + at, garbage, size); at += size;
    memcpy(answer + at, good, sizeof(good)); at += sizeof(good);

    ScriptedBroker broker(answer, at);
    MQTTClient mqtt;
    g_delivered = 0;
    g_last_data.clear();
    mqtt.OnData(countDelivery);

    connectAndRead(mqtt, broker);
    for (int i = 0; i < 20 && "good" != g_last_data; i++)
    {
        mqtt.MQTT_Task();
    }
    return "good" == g_last_data;
}

} // namespace

TEST(mqttclient, a_publish_whose_topic_runs_past_the_packet_is_not_delivered)
{
    static const uint8_t bad[] = {0x30, 0x04, 0xFF, 0xFF, 'a', 'b'};
    g_delivered = 0;
    deliveredAfter(bad, sizeof(bad));
    ASSERT_TRUE(g_delivered <= 1u);
}

TEST(mqttclient, a_packet_of_a_reserved_type_is_survived)
{
    static const uint8_t reserved[] = {0xF0, 0x00};
    static const uint8_t zero[] = {0x00, 0x00};
    ASSERT_TRUE(deliveredAfter(reserved, sizeof(reserved)));
    ASSERT_TRUE(deliveredAfter(zero, sizeof(zero)));
}

TEST(mqttclient, a_remaining_length_of_five_bytes_is_survived)
{
    static const uint8_t bad[] = {0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0x01};
    deliveredAfter(bad, sizeof(bad));
}

TEST(mqttclient, a_publish_bigger_than_the_buffer_does_not_garble_the_next)
{
    static uint8_t big[MQTT_BUF_SIZE * 2];
    uint32_t payload = sizeof(big) - 7;
    uint32_t remaining = payload + 3;
    big[0] = 0x30;
    big[1] = (uint8_t)(0x80 | (remaining & 0x7F));
    big[2] = (uint8_t)(remaining >> 7);
    big[3] = 0x00; big[4] = 0x01; big[5] = 't';
    memset(big + 6, 'x', sizeof(big) - 6);

    ASSERT_TRUE(deliveredAfter(big, 6 + payload));
}

TEST(mqttclient, a_qos2_release_from_the_broker_is_completed)
{
    static const uint8_t answer[] = {
        0x20, 0x02, 0x00, 0x00,
        0x34, 0x07, 0x00, 0x01, 't', 0x03, 0x09, 'h', 'i',
        0x62, 0x02, 0x03, 0x09,
    };
    ScriptedBroker broker(answer, sizeof(answer));
    MQTTClient mqtt;
    g_delivered = 0;
    mqtt.OnData(countDelivery);

    connectAndRead(mqtt, broker);
    for (int i = 0; i < 10; i++)
    {
        mqtt.MQTT_Task();
    }

    ASSERT_EQ(g_delivered, 1u);
    ASSERT_TRUE(broker.sawWritten(0x70, 0x0309));
}

TEST(mqttclient, a_message_from_the_broker_is_answered_while_the_client_has_its_own_queued)
{
    static const uint8_t answer[] = {
        0x20, 0x02, 0x00, 0x00,
        0x32, 0x07, 0x00, 0x01, 't', 0x00, 0x05, 'h', 'i',
    };
    ScriptedBroker broker(answer, sizeof(answer));
    broker.m_limit = 4;
    MQTTClient mqtt;
    g_delivered = 0;
    mqtt.OnData(countDelivery);

    connectAndRead(mqtt, broker);
    ASSERT_EQ((int)mqtt.m_mqttClient.connState, (int)MQTT_DATA);
    ASSERT_TRUE(mqtt.Publish("a", "1", 1, 0, 0));
    ASSERT_TRUE(mqtt.Publish("b", "2", 1, 0, 0));

    broker.m_limit = sizeof(answer);
    mqtt.MQTT_Task();

    ASSERT_EQ(g_delivered, 1u);
    ASSERT_TRUE(broker.sawWritten(0x40, 0x0005));
}

namespace {

/**
 * Whether the client wrote a ping request.
 */
bool sawPing(const ScriptedBroker &broker)
{
    for (size_t i = 0; i + 1 < broker.m_written.size(); i++)
    {
        if (0xC0 == broker.m_written[i] && 0x00 == broker.m_written[i + 1]) return true;
    }
    return false;
}

} // namespace

TEST(mqttclient, the_keepalive_ping_goes_out_after_the_same_time_at_any_tick)
{
    static const uint8_t answer[] = { 0x20, 0x02, 0x00, 0x00 };
    ScriptedBroker broker(answer, sizeof(answer));
    MQTTClient mqtt;

    connectAndRead(mqtt, broker);
    ASSERT_EQ((int)mqtt.m_mqttClient.connState, (int)MQTT_DATA);

    uint32_t due = MQTT_MS_TO_TICKS(8 * MILLISECOND_DURATION_1000);
    ASSERT_TRUE(due * MQTT_TIMER_INTERVAL_MS >= 8 * MILLISECOND_DURATION_1000);

    for (uint32_t i = 0; i < due; i++)
    {
        mqtt.mqtt_timer();
    }
    ASSERT_FALSE(sawPing(broker));

    mqtt.mqtt_timer();
    ASSERT_TRUE(sawPing(broker));
    ASSERT_EQ((int)mqtt.m_mqttClient.connState, (int)MQTT_PING_SENT);
}

static int g_timeouts = 0;

static void countTimeout(uint32_t *args)
{
    g_timeouts++;
}

TEST(mqttclient, an_unanswered_connect_times_out_after_the_same_time_at_any_tick)
{
    static const uint8_t answer[] = { 0x00 };
    ScriptedBroker broker(answer, 0);
    MQTTClient mqtt;
    g_timeouts = 0;
    mqtt.OnTimeout(countTimeout);

    connectAndRead(mqtt, broker);
    ASSERT_EQ((int)mqtt.m_mqttClient.connState, (int)MQTT_CONNECT_SENT);

    uint32_t due = MQTT_MS_TO_TICKS(MQTT_READ_TIMEOUT_MS);
    ASSERT_TRUE(due * MQTT_TIMER_INTERVAL_MS >= MQTT_READ_TIMEOUT_MS);

    for (uint32_t i = 0; i < due; i++)
    {
        mqtt.mqtt_timer();
    }
    ASSERT_EQ(g_timeouts, 0);

    mqtt.mqtt_timer();
    ASSERT_EQ(g_timeouts, 1);
}

TEST(mqttclient, an_unanswered_ping_reconnects_after_the_read_timeout)
{
    static const uint8_t answer[] = { 0x20, 0x02, 0x00, 0x00 };
    ScriptedBroker broker(answer, sizeof(answer));
    MQTTClient mqtt;
    g_timeouts = 0;
    mqtt.OnTimeout(countTimeout);

    connectAndRead(mqtt, broker);
    mqtt.m_mqttClient.connState = MQTT_KEEPALIVE_REQ;
    mqtt.MQTT_Task();
    ASSERT_TRUE(sawPing(broker));

    uint32_t due = MQTT_MS_TO_TICKS(MQTT_READ_TIMEOUT_MS);
    for (uint32_t i = 0; i < due; i++)
    {
        mqtt.mqtt_timer();
    }
    ASSERT_EQ(g_timeouts, 0);

    mqtt.mqtt_timer();
    ASSERT_EQ(g_timeouts, 1);
}

TEST(mqttclient, an_answered_ping_does_not_reconnect)
{
    static const uint8_t answer[] = { 0x20, 0x02, 0x00, 0x00, 0xD0, 0x00 };
    ScriptedBroker broker(answer, sizeof(answer));
    broker.m_limit = 4;
    MQTTClient mqtt;
    g_timeouts = 0;
    mqtt.OnTimeout(countTimeout);

    connectAndRead(mqtt, broker);
    mqtt.m_mqttClient.connState = MQTT_KEEPALIVE_REQ;
    mqtt.MQTT_Task();
    ASSERT_TRUE(sawPing(broker));

    broker.m_limit = sizeof(answer);
    mqtt.mqtt_timer();
    ASSERT_FALSE(mqtt.m_mqttClient.m_ping_awaited);

    uint32_t past = MQTT_MS_TO_TICKS(MQTT_READ_TIMEOUT_MS) + 3;
    for (uint32_t i = 0; i < past; i++)
    {
        mqtt.mqtt_timer();
    }
    ASSERT_EQ(g_timeouts, 0);
}

TEST(mqttclient, an_acknowledgement_the_client_sends_does_not_leave_it_waiting)
{
    static const uint8_t answer[] = {
        0x20, 0x02, 0x00, 0x00,
        0x32, 0x07, 0x00, 0x01, 't', 0x00, 0x05, 'h', 'i',
    };
    ScriptedBroker broker(answer, sizeof(answer));
    MQTTClient mqtt;
    g_delivered = 0;
    mqtt.OnData(countDelivery);

    connectAndRead(mqtt, broker);
    for (int i = 0; i < 4; i++)
    {
        mqtt.MQTT_Task();
    }

    ASSERT_TRUE(broker.sawWritten(0x40, 0x0005));
    ASSERT_EQ((int)mqtt.m_mqttClient.connState, (int)MQTT_DATA);
}

TEST(mqttclient, an_empty_payload_is_a_message_not_a_malformed_packet)
{
    static const uint8_t answer[] = {
        0x20, 0x02, 0x00, 0x00,
        0x30, 0x03, 0x00, 0x01, 't',
        0x32, 0x05, 0x00, 0x01, 't', 0x00, 0x06,
    };
    ScriptedBroker broker(answer, sizeof(answer));
    MQTTClient mqtt;
    g_delivered = 0;
    mqtt.OnData(countDelivery);

    connectAndRead(mqtt, broker);
    for (int i = 0; i < 6; i++)
    {
        mqtt.MQTT_Task();
    }

    ASSERT_EQ(g_delivered, 2u);
    ASSERT_TRUE(broker.sawWritten(0x40, 0x0006));
}

TEST(mqttclient, every_refusal_code_is_a_failure)
{
    for (uint8_t code = 1; code <= 5; code++)
    {
        uint8_t answer[] = {0x20, 0x02, 0x00, code};
        ScriptedBroker broker(answer, sizeof(answer));
        MQTTClient mqtt;

        connectAndRead(mqtt, broker);

        ASSERT_EQ((int)mqtt.m_mqttClient.connState, (int)MQTT_CONNECT_FAILED);
        ASSERT_FALSE(mqtt.is_mqtt_connected());
    }
}

#endif
