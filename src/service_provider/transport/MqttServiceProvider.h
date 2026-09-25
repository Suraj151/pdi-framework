/****************************** Mqtt service **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MQTT_SERVICE_PROVIDER_H_
#define _MQTT_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>
#include <service_provider/database/DatabaseServiceProvider.h>

#ifdef ENABLE_GPIO_SERVICE
#include <service_provider/device/GpioServiceProvider.h>
#endif
#include <transports/mqtt/MqttClient.h>

typedef pdiutil::function<void(char *, uint16_t)> MqttPublishDataCallback;
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

  bool initService(void *arg = nullptr) override;

  /**
   * Needs its configuration, and the network it talks over, before it can
   * come up.
   */
  uint8_t getServiceDependencies(service_t *_out, uint8_t _max) const override {
    uint8_t _n = 0;
    if( _max > _n ) _out[_n++] = SERVICE_DATABASE;
  #ifdef ENABLE_WIFI_SERVICE
    if( _max > _n ) _out[_n++] = SERVICE_WIFI;
  #endif
    return _n;
  }

  void handleMqttPublish(bool sync=false);
  void handleMqttSubScribe(void);

  /**
   * Copy what the next publish should carry into the buffer this service owns,
   * refusing while it holds none and when the text does not fit.
   */
  bool setMqttPayload(const char *data, uint16_t len);

  /**
   * Build the transport the configured scheme asks for, keeping the one already
   * held when it is of the right kind.
   */
  bool prepareClientForConfig(mqtt_general_config_table *_general);
  void handleMqttConfigChange(int _mqtt_config_type = MQTT_GENERAL_CONFIG);
  static void handleMqttDataCb(uint32_t *args, const char *topic, uint32_t topic_len, const char *data, uint32_t data_len);
  void setMqttPublishDataCallback(MqttPublishDataCallback _cb);
  void setMqttSubscribeDataCallback(MqttSubscribeDataCallback _cb);
  void stop(void);

  /**
   * Drops the broker connection and releases the client and payload buffer a
   * run allocated, so a stopped mqtt service holds nothing.
   */
  bool stopService() override;

  /**
   * Forget the task ids, which name tasks the stop has already dropped.
   */
  void resetServiceState() override;

  void printConfigToTerminal(iTerminalInterface *terminal) override;

  /**
   * @var	int|0 m_mqtt_timer_cb_id
   */
  pdiutil::task_id_t m_mqtt_timer_cb_id;
  /**
   * @var	int16_t|0 m_mqtt_publish_cb_id
   */
  pdiutil::task_id_t m_mqtt_publish_cb_id;
  /**
   * @var	int16_t|0 m_mqtt_subscribe_cb_id
   */
  pdiutil::task_id_t m_mqtt_subscribe_cb_id;
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

  /**
   * @array	char  m_mqtt_payload
   */
  char *m_mqtt_payload;
};

extern MqttServiceProvider __mqtt_service;

#endif
