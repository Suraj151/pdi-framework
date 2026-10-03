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

#ifdef ENABLE_MQTT_SERVICE

namespace {

/**
 * A connection that answers with scripted bytes, zeros included, and records
 * whether the client hung up.
 */
class ScriptedBroker : public iClientInterface
{

public:

    ScriptedBroker(const uint8_t *answer, uint32_t size) : m_answer(answer), m_size(size), m_at(0), m_connected(0) {}

    int16_t connect(const uint8_t *host, uint16_t port) override { m_connected = 1; return 1; }
    int16_t disconnect() override { m_connected = 0; return 1; }
    int8_t connected() override { return m_connected; }
    void setTimeout(uint32_t timeout) override {}

    int32_t write(uint8_t c) override { return 1; }
    int32_t write(const uint8_t *c_str) override { return (int32_t)strlen((const char *)c_str); }
    int32_t write(const uint8_t *c_str, uint32_t size) override { return (int32_t)size; }

    int32_t available() override { return (int32_t)(m_size - m_at); }

    uint8_t read() override
    {
        if (m_at >= m_size) return 0;
        return m_answer[m_at++];
    }

    int32_t read(uint8_t *buf, uint32_t size) override
    {
        uint32_t taken = 0;
        while (taken < size && m_at < m_size) {
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
