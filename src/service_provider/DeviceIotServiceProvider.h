/************************* Device IOT service *********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _DEVICE_IOT_SERVICE_PROVIDER_H_
#define _DEVICE_IOT_SERVICE_PROVIDER_H_


#include <service_provider/ServiceProvider.h>
#include <service_provider/MqttServiceProvider.h>
#include <service_provider/DatabaseServiceProvider.h>
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

    void init( iClientInterface *_iclient );
    void handleRegistrationOtpRequest(  device_iot_config_table *_device_iot_configs, std::string &_response  );
    void handleDeviceIotConfigRequest( void );
    void handleConnectivityCheck( void );
    void configureMQTT( void );
    void handleServerConfigurableParameters(char *json_resp);
    void beginSensorData( void );
    void handleSensorData( void );
    void initDeviceIotSensor( iDeviceIotInterface *_device );
    void printDeviceIotConfigLogs( void );

  protected:

    device_iot_config_table m_device_iot_configs;

    bool      m_token_validity;

    uint16_t  m_smaple_index;
    uint16_t  m_smaple_per_publish;
    uint16_t  m_sensor_data_publish_freq;
    uint16_t  m_mqtt_keep_alive;

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
