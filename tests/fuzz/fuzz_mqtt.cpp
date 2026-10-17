/******************************** Fuzz MQTT ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The mqtt client reading what a broker sends it: the fixed header and its
remaining length, then every packet type the client dispatches on — CONNACK,
PUBLISH at each qos, the acknowledgements, SUBACK — with the topic and payload
handed to the data callback. Whoever runs the broker, or sits between it and
the board, chooses every one of these bytes, and one malformed PUBLISH used to
send a null topic with the packet's whole length into a copy.

Author          : Suraj I.
created Date    : 9th Oct 2026
******************************************************************************/

#include <FuzzCommon.h>

#ifdef ENABLE_MQTT_SERVICE

#include <transports/mqtt/MqttClient.h>

namespace
{

    volatile uint32_t s_seen = 0;

    void touch(uint32_t *args, const char *topic, uint32_t topic_len, const char *data, uint32_t data_len)
    {
        for (uint32_t i = 0; i < topic_len; i++) s_seen += (uint8_t)topic[i];
        for (uint32_t i = 0; i < data_len; i++) s_seen += (uint8_t)data[i];
    }

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    pdifuzz::useFreeClock();

    static uint8_t stream[4096 + 4];
    static const uint8_t connack[] = {0x20, 0x02, 0x00, 0x00};
    size_t body = size > 4096 ? 4096 : size;
    memcpy(stream, connack, sizeof(connack));
    memcpy(stream + sizeof(connack), data, body);

    pdifuzz::FuzzClient client(stream, sizeof(connack) + body);

    mqtt_general_config_table general;
    mqtt_lwt_config_table lwt;
    strcpy(general.host, "broker.example");
    general.port = 1883;
    strcpy(general.client_id, "fuzz");
    general.keepalive = 10;

    MQTTClient mqtt;
    mqtt.OnData(touch);
    mqtt.begin(&client, &general, &lwt);
    for (int i = 0; i < 32 && client.available() > 0; i++)
    {
        mqtt.MQTT_Task();
    }

    return 0;
}

#else

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    return 0;
}

#endif
