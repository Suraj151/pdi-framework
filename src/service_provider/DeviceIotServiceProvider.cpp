/*************************** Device IOT service *******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include <config/Config.h>

#if defined(ENABLE_DEVICE_IOT)

#include "DeviceIotServiceProvider.h"

/**
 * DeviceIotServiceProvider constructor.
 */
DeviceIotServiceProvider::DeviceIotServiceProvider():
  m_token_validity(false),
  m_smaple_index(0),
  m_smaple_per_publish(SENSOR_DATA_SAMPLING_PER_PUBLISH),
  m_sensor_data_publish_freq(SENSOR_DATA_PUBLISH_FREQ),
  m_mqtt_keep_alive(MQTT_DEFAULT_KEEPALIVE),
  m_handle_sensor_data_cb_id(0),
  m_mqtt_connection_check_cb_id(0),
  m_device_config_request_cb_id(0),
  m_device_iot(nullptr),
  m_http_client(Http_Client::GetStaticInstance())
{
}

/**
 * DeviceIotServiceProvider destructor
 */
DeviceIotServiceProvider::~DeviceIotServiceProvider(){
  this->m_device_iot = nullptr;
  this->m_http_client = nullptr;
}

/**
 * start device registration services if enabled
 */
void DeviceIotServiceProvider::init( iClientInterface *_iclient ){

  if( nullptr != this->m_http_client ){
    this->m_http_client->SetClient(_iclient);
  }

  // __task_scheduler.setInterval( [&]() { this->handleDeviceIotConfigRequest(); }, HTTP_REQUEST_DURATION, __i_dvc_ctrl.millis_now() );
  this->m_device_config_request_cb_id = __task_scheduler.updateInterval(
    this->m_device_config_request_cb_id,
    [&]() {
      this->handleDeviceIotConfigRequest();
    },
    this->m_mqtt_keep_alive*MILLISECOND_DURATION_1000
  );

  // clear all mqtt old configs
  __mqtt_general_table.clear();
  __mqtt_pubsub_table.clear();
  __mqtt_lwt_table.clear();
}

/**
 * handle registration otp request
 */
void DeviceIotServiceProvider::handleRegistrationOtpRequest( device_iot_config_table *_device_iot_configs, String &_response ){

  std::string otpurl;

  if( nullptr != _device_iot_configs ){

    otpurl = _device_iot_configs->device_iot_host;
    otpurl += DEVICE_IOT_OTP_REQ_URL;

    size_t mac_index = otpurl.find("[mac]");
    if( std::string::npos != mac_index )
    {
      otpurl.replace( mac_index, 5, __i_dvc_ctrl.getDeviceMac().c_str() );
    }
  }

  LogFmtI("Handling device otp Http Request : %s\n", otpurl.c_str());

  if( otpurl.size() > 5 && nullptr != this->m_http_client ){

    this->m_http_client->Begin();
    this->m_http_client->SetUserAgent("esp");
    this->m_http_client->SetBasicAuthorization("iot-otp", __i_dvc_ctrl.getDeviceMac().c_str());
    this->m_http_client->SetTimeout(2*MILLISECOND_DURATION_1000);

    int _httpCode = this->m_http_client->Get(otpurl.c_str());
    
    char *http_resp = nullptr;
    int16_t httl_resp_len = 0;
    this->m_http_client->GetResponse( http_resp, httl_resp_len );

    LogFmtI("Http device otp Response code : %d\n", _httpCode );
    if ( _httpCode == HTTP_RESP_OK && nullptr != http_resp && httl_resp_len <= DEVICE_IOT_OTP_API_RESP_LENGTH ) {

      _response = http_resp;
    }else{

      _response = "{\"status\":false,\"remark\":";
      _response += "\"Device request failed. ErrCode(";
      _response += _httpCode;
      _response += ") !\"}";
    }

    this->m_http_client->End(true);
  }else{
    LogE("Device otp Request not initializing or failed or Not Configured Correctly\n");
  }
}

/**
 * handle config request
 */
void DeviceIotServiceProvider::handleDeviceIotConfigRequest(){

  // do not proceed if already validated
  if( this->m_token_validity ) {
    return;
  }

  __database_service.get_device_iot_config_table(&this->m_device_iot_configs);

  std::string configurl = this->m_device_iot_configs.device_iot_host;
  configurl += DEVICE_IOT_CONFIG_REQ_URL;

  size_t mac_index = configurl.find("[mac]");
  if( std::string::npos != mac_index )
  {
    configurl.replace( mac_index, 5, __i_dvc_ctrl.getDeviceMac().c_str() );
  }

  LogFmtI("Handling device iot config Request : %s\n", configurl.c_str());

  if( configurl.size() > 5 && nullptr != this->m_http_client ){

    this->m_http_client->Begin();
    this->m_http_client->SetUserAgent("esp");
    this->m_http_client->SetBasicAuthorization("iot-otp", __i_dvc_ctrl.getDeviceMac().c_str());
    this->m_http_client->SetTimeout(2*MILLISECOND_DURATION_1000);
    int _httpCode = this->m_http_client->Get(configurl.c_str());

    char *http_resp = nullptr;
    int16_t httl_resp_len = 0;
    this->m_http_client->GetResponse( http_resp, httl_resp_len );

    if ( _httpCode == HTTP_RESP_OK && nullptr != http_resp ) {

      if( httl_resp_len < DEVICE_IOT_CONFIG_RESP_MAX_SIZE ){

        if( 0 <= __strstr( http_resp, (char*)DEVICE_IOT_CONFIG_TOKEN_KEY, DEVICE_IOT_CONFIG_RESP_MAX_SIZE - strlen(DEVICE_IOT_CONFIG_TOKEN_KEY) ) ){

          bool _json_result = __get_from_json( http_resp, (char*)DEVICE_IOT_CONFIG_TOKEN_KEY, this->m_device_iot_configs.device_iot_token, DEVICE_IOT_CONFIG_TOKEN_MAX_SIZE );
          _json_result = __get_from_json( http_resp, (char*)DEVICE_IOT_CONFIG_TOPIC_KEY, this->m_device_iot_configs.device_iot_topic, DEVICE_IOT_CONFIG_TOPIC_MAX_SIZE );
          if(  _json_result && strlen( this->m_device_iot_configs.device_iot_token ) && strlen( this->m_device_iot_configs.device_iot_topic ) ){

            LogFmtI("Got Token : %s\n", this->m_device_iot_configs.device_iot_token );
            LogFmtI("Got Topic : %s\n", this->m_device_iot_configs.device_iot_topic );
            __database_service.set_device_iot_config_table( &this->m_device_iot_configs );

            this->handleServerConfigurableParameters( http_resp );

            this->m_mqtt_connection_check_cb_id = __task_scheduler.updateInterval(
              this->m_mqtt_connection_check_cb_id,
              [&]() {
                this->handleConnectivityCheck();
              },
              this->m_mqtt_keep_alive*MILLISECOND_DURATION_1000, DEFAULT_TASK_PRIORITY,
              ( __i_dvc_ctrl.millis_now() + MQTT_INITIALIZE_DURATION )
            );
            __task_scheduler.setTimeout( [&]() { this->configureMQTT(); }, 1, __i_dvc_ctrl.millis_now() );
            this->m_token_validity = true;

          }else{

            this->m_token_validity = false;
          }
        }else{

          this->m_token_validity = false;
        }
      }else{
        LogW("Http Response : response size over !\n");
      }
    }

    this->m_http_client->End(true);
  }else{

    LogE("Device iot config request not initializing or failed or Not Configured Correctly\n");
  }
}

/**
 * handle mqtt connections checks cycle
 */
void DeviceIotServiceProvider::handleConnectivityCheck(){

  bool _is_mqtt_connected = __mqtt_service.m_mqtt_client.is_mqtt_connected();
  LogFmtI("Device iot mqtt connection check cycle : %d\n", (int)_is_mqtt_connected );
  if( !_is_mqtt_connected ) {
    this->m_token_validity = false;
    __mqtt_service.stop();
  }
}


/**
 * configure mqtt
 */
void DeviceIotServiceProvider::configureMQTT(){

  mqtt_general_config_table _mqtt_general_configs;
  mqtt_lwt_config_table _mqtt_lwt_configs;
  mqtt_pubsub_config_table _mqtt_pubsub_configs;
  __database_service.get_mqtt_general_config_table(&_mqtt_general_configs);
  __database_service.get_mqtt_lwt_config_table(&_mqtt_lwt_configs);
  __database_service.get_mqtt_pubsub_config_table(&_mqtt_pubsub_configs);

  memset( &_mqtt_general_configs, 0, sizeof(mqtt_general_config_table));
  memset( &_mqtt_pubsub_configs, 0, sizeof(mqtt_pubsub_config_table));
  memset( &_mqtt_lwt_configs, 0, sizeof(mqtt_lwt_config_table));

  // parse the host from iot host config
  httt_req_t httpreq; httpreq.init(this->m_device_iot_configs.device_iot_host);

  memcpy( _mqtt_general_configs.host, httpreq.host, strlen( httpreq.host ) );
  _mqtt_general_configs.port = DEVICE_IOT_MQTT_DATA_PORT;
  strcpy_P( _mqtt_general_configs.client_id, PSTR("[mac]") );
  strcpy_P( _mqtt_general_configs.username, PSTR("[mac]") );
  memcpy( _mqtt_general_configs.password, this->m_device_iot_configs.device_iot_token, DEVICE_IOT_CONFIG_TOKEN_MAX_SIZE );
  _mqtt_general_configs.keepalive = this->m_mqtt_keep_alive;
  _mqtt_general_configs.clean_session = 1;

  strcpy( _mqtt_pubsub_configs.publish_topics[0].topic, this->m_device_iot_configs.device_iot_topic );
  // _mqtt_pubsub_configs.publish_frequency = this->m_sensor_data_publish_freq;

  strcpy( _mqtt_lwt_configs.will_topic, this->m_device_iot_configs.device_iot_topic );
  // strcpy_P( _mqtt_lwt_configs.will_topic, DEVICE_IOT_MQTT_WILL_TOPIC );
  strcpy_P( _mqtt_lwt_configs.will_message, PSTR("{\"head\":{\"uid\":\"[mac]\",\"packet_type\":\"disconnect\"},\"payload\":{},\"tail\":{}}") );
  _mqtt_lwt_configs.will_qos = 1;
  _mqtt_lwt_configs.will_retain = 0;

  __database_service.set_mqtt_general_config_table( &_mqtt_general_configs );
  __database_service.set_mqtt_lwt_config_table( &_mqtt_lwt_configs );
  __database_service.set_mqtt_pubsub_config_table( &_mqtt_pubsub_configs );

  __task_scheduler.setTimeout( [&]() { __mqtt_service.handleMqttConfigChange(); }, 1, __i_dvc_ctrl.millis_now() );
}

/**
 * handle mqtt config parameters from server
 */
void DeviceIotServiceProvider::handleServerConfigurableParameters(char* json_resp){

  char *_value_buff = new char[10];

  memset( _value_buff, 0, 10 );
  bool _json_result = __get_from_json( json_resp, (char*)DEVICE_IOT_CONFIG_DATA_RATE_KEY, _value_buff, 6 );
  uint16_t data_rate = StringToUint16( _value_buff, 6 );
  if( _json_result && SENSOR_DATA_PUBLISH_FREQ_MIN_LIMIT <= data_rate && data_rate <= SENSOR_DATA_PUBLISH_FREQ_MAX_LIMIT ){

    this->m_sensor_data_publish_freq = data_rate;
    LogFmtI("Got Data rate : %d\n", data_rate);
  }

  uint16_t sample_rate = round ( this->m_sensor_data_publish_freq / ( ( SENSOR_DATA_SAMPLES_PER_PUBLISH_MAX_LIMIT * 0.125 ) * ( log(this->m_sensor_data_publish_freq) ) ) );
  if( 0 < sample_rate && sample_rate <= SENSOR_DATA_SAMPLES_PER_PUBLISH_MAX_LIMIT ){

    this->m_smaple_per_publish = sample_rate;
    LogFmtI("Got Sample rate : %d\n", sample_rate);
  }

  memset( _value_buff, 0, 10 );
  _json_result = __get_from_json( json_resp, (char*)DEVICE_IOT_CONFIG_MQTT_KEEP_ALIVE_KEY, _value_buff, 6 );
  uint16_t keep_alive = StringToUint16( _value_buff, 6 );
  if( _json_result && DEVICE_IOT_MQTT_KEEP_ALIVE_MIN <= keep_alive && keep_alive <= DEVICE_IOT_MQTT_KEEP_ALIVE_MAX ){

    this->m_mqtt_keep_alive = keep_alive;
    LogFmtI("Got keep alive : %d\n", keep_alive);
  }

  this->beginSensorData();

  this->m_device_config_request_cb_id = __task_scheduler.updateInterval(
    this->m_device_config_request_cb_id,
    [&]() {
      this->handleDeviceIotConfigRequest();
    },
    this->m_mqtt_keep_alive*MILLISECOND_DURATION_1000
  );

  delete[] _value_buff;
}



/**
 * begin sensor data processs
 */
void DeviceIotServiceProvider::beginSensorData(){

  this->m_handle_sensor_data_cb_id = __task_scheduler.updateInterval(
    this->m_handle_sensor_data_cb_id,
    [&]() { this->handleSensorData(); },
    (((float)this->m_sensor_data_publish_freq/(float)this->m_smaple_per_publish)*1000.0)
  );
  this->m_smaple_index = 0;
  if( nullptr != this->m_device_iot ){
    this->m_device_iot->resetSampleHook();
  }
}

/**
 * init sensor device processs
 */
void DeviceIotServiceProvider::initDeviceIotSensor( iDeviceIotInterface *_device ){
  this->m_device_iot = _device;
  if( nullptr != this->m_device_iot ){
    this->beginSensorData();
  }
}

/**
 * handle sensor data. takes defined samples and average them to send.
 */
void DeviceIotServiceProvider::handleSensorData(){

  LogFmtI("Handling sensor data samples: %d\n", this->m_smaple_index);

  if( nullptr == this->m_device_iot ){
    return;
  }

  this->m_device_iot->sampleHook();

  if( this->m_smaple_index >= this->m_smaple_per_publish-1 ){

    this->m_smaple_index = 0;
    String _payload = "{\"head\":{\"uid\":\"[mac]\",\"packet_type\":\"data\",\"packet_version\":\"";
    _payload += DEVICE_IOT_PACKET_VERSION;
    _payload += "\"},\"payload\":";
    this->m_device_iot->dataHook(_payload);
    _payload += ",\"tail\":{}}";
    __task_scheduler.setTimeout( [&]() { __mqtt_service.handleMqttPublish(); }, 1, __i_dvc_ctrl.millis_now() );

    memset( __mqtt_service.m_mqtt_payload, 0, MQTT_PAYLOAD_BUF_SIZE );
    if( _payload.length()+1 < MQTT_PAYLOAD_BUF_SIZE ){
      _payload.toCharArray( __mqtt_service.m_mqtt_payload, _payload.length()+1);
    }else{
      strcat_P( __mqtt_service.m_mqtt_payload, PSTR("mqtt data is too big to fit in buffer !"));
    }
  }else{
    this->m_smaple_index++;
  }
}

/**
 * print device register configs
 */
void DeviceIotServiceProvider::printDeviceIotConfigLogs(){

  device_iot_config_table _device_iot_configs;
  __database_service.get_device_iot_config_table(&_device_iot_configs);

  LogFmtI("\nDevice IOT Configs : %s\n\n", _device_iot_configs.device_iot_host);
}

DeviceIotServiceProvider __device_iot_service;

#endif
