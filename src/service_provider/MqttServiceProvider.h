/****************************** Mqtt service **********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MQTT_SERVICE_PROVIDER_H_
#define _MQTT_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>
#include <service_provider/DatabaseServiceProvider.h>

#ifdef ENABLE_GPIO_SERVICE
#include <service_provider/GpioServiceProvider.h>
#endif
#include <transports/mqtt/MqttClient.h>

#define MQTT_PAYLOAD_BUF_SIZE 400

typedef std::function<void(char *, uint16_t)> MqttPublishDataCallback;
typedef MqttDataCallback MqttSubscribeDataCallback;

/**
 * MqttServiceProvider class
 */
class MqttServiceProvider : public ServiceProvider
{

public:
  /**
   * MqttServiceProvider constructor.
   */
  MqttServiceProvider();
  /**
   * MqttServiceProvider destructor
   */
  ~MqttServiceProvider();

  void begin( iClientInterface* _client );
  void handleMqttPublish(void);
  void handleMqttSubScribe(void);
  void handleMqttConfigChange(int _mqtt_config_type = MQTT_GENERAL_CONFIG);
  static void handleMqttDataCb(uint32_t *args, const char *topic, uint32_t topic_len, const char *data, uint32_t data_len);
  void setMqttPublishDataCallback(MqttPublishDataCallback _cb);
  void setMqttSubscribeDataCallback(MqttSubscribeDataCallback _cb);
  void stop(void);
  void printMqttConfigLogs(void);

  /**
   * @var	int|0 m_mqtt_timer_cb_id
   */
  int m_mqtt_timer_cb_id;
  /**
   * @var	int|0 m_mqtt_publish_cb_id
   */
  int m_mqtt_publish_cb_id;
  /**
   * @var	int|0 m_mqtt_subscribe_cb_id
   */
  int m_mqtt_subscribe_cb_id;
  /**
   * @array	char  m_mqtt_payload
   */
  char *m_mqtt_payload;
  /**
   * @var	MqttPublishDataCallback  m_mqtt_publish_data_cb
   */
  MqttPublishDataCallback m_mqtt_publish_data_cb;
  /**
   * @var	MqttSubscribeDataCallback  m_mqtt_subscribe_data_cb
   */
  MqttSubscribeDataCallback m_mqtt_subscribe_data_cb;
  /**
   * @var	MQTTClient  m_mqtt_client
   */
  MQTTClient m_mqtt_client;
  /**
   * @var	iClientInterface*  m_client
   */
  iClientInterface *m_client;

protected:

};

extern MqttServiceProvider __mqtt_service;

#endif
