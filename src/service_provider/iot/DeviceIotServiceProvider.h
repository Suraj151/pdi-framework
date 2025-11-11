/************************* Device IOT service *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _DEVICE_IOT_SERVICE_PROVIDER_H_
#define _DEVICE_IOT_SERVICE_PROVIDER_H_


#include <service_provider/ServiceProvider.h>
#include <service_provider/transport/MqttServiceProvider.h>
#include <service_provider/database/DatabaseServiceProvider.h>
#include <transports/http/HTTPClient.h>

/**
 * DeviceIotServiceProvider class
 */
class DeviceIotServiceProvider : public ServiceProvider {

  public:

    /**
     * DeviceIotServiceProvider constructor.
     */
    DeviceIotServiceProvider();
    /**
		 * DeviceIotServiceProvider destructor
		 */
    ~DeviceIotServiceProvider();

    bool initService(void *arg = nullptr) override;
    void handleRegistrationOtpRequest(  device_iot_config_table *_device_iot_configs, pdiutil::string &_response  );
    void handleDeviceIotConfigRequest( void );
    void handleConnectivityCheck( void );
#if defined(ENABLE_MQTT_SERVICE)
    void configureMQTT( void );
    static void handleSubscribeCallback( uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len );
#endif
    void handleServerConfigurableParameters(char *json_resp);
    void beginSensorData( void );
    void handleSensorData( void );
    void initDeviceIotSensor( iDeviceIotInterface *_device );
    void printConfigToTerminal(iTerminalInterface *terminal) override;

    /**
		 * @var	variables received from server config
		 */
    uint64_t  m_server_configurable_device_id;
    uint16_t  m_server_configurable_mqtt_keep_alive;
    uint16_t  m_server_configurable_sample_per_publish;
    uint16_t  m_server_configurable_sensor_data_publish_freq;

    char      m_server_configurable_channel_host[DEVICE_IOT_CONFIG_CHANNEL_MAX_BUFF_SIZE];
    uint32_t  m_server_configurable_channel_port;
    char      m_server_configurable_channel_read[DEVICE_IOT_CONFIG_CHANNEL_MAX_BUFF_SIZE];
    char      m_server_configurable_channel_write[DEVICE_IOT_CONFIG_CHANNEL_MAX_BUFF_SIZE];
    char      m_server_configurable_channel_token[DEVICE_IOT_CONFIG_CHANNEL_TOKEN_MAX_SIZE];

    pdiutil::vector<pdiutil::string> m_server_configurable_interface_read;
    pdiutil::vector<pdiutil::string> m_server_configurable_interface_write;

    bool      m_handle_channel_write_asap;

  protected:

    device_iot_config_table m_device_iot_configs;
    bool      m_token_validity;
    uint16_t  m_sample_index;

    int16_t   m_handle_sensor_data_cb_id;
    int16_t   m_mqtt_connection_check_cb_id;
    int16_t   m_device_config_request_cb_id;

    /**
		 * @var	iDeviceIotInterface*  m_device_iot
		 */
    iDeviceIotInterface   *m_device_iot;
    /**
     * @var	Http_Client  *m_http_client
     */
    Http_Client           *m_http_client;
};

extern DeviceIotServiceProvider __device_iot_service;

#endif
