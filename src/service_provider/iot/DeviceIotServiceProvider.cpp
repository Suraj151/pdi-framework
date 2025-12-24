/*************************** Device IOT service *******************************
This file is part of the pdi stack.

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
  m_sample_index(0),
  m_server_configurable_device_id(0),
  m_server_configurable_sample_per_publish(SENSOR_DATA_SAMPLING_PER_PUBLISH),
  m_server_configurable_sensor_data_publish_freq(SENSOR_DATA_PUBLISH_FREQ),
  m_server_configurable_channel_port(DEVICE_IOT_DEFAULT_CHANNEL_DATA_PORT),
  m_server_configurable_mqtt_keep_alive(MQTT_DEFAULT_KEEPALIVE),
  m_handle_channel_write_asap(false),
  m_handle_sensor_data_cb_id(0),
  m_mqtt_connection_check_cb_id(0),
  m_device_config_request_cb_id(0),
  m_device_iot(nullptr),
  m_http_client(Http_Client::GetStaticInstance()),
  ServiceProvider(SERVICE_DVCIOT, RODT_ATTR("IOT"))
{
  memset(m_server_configurable_channel_host, 0, DEVICE_IOT_CONFIG_CHANNEL_MAX_BUFF_SIZE);
  memset(m_server_configurable_channel_write, 0, DEVICE_IOT_CONFIG_CHANNEL_MAX_BUFF_SIZE);
  memset(m_server_configurable_channel_read, 0, DEVICE_IOT_CONFIG_CHANNEL_MAX_BUFF_SIZE);
  memset(m_server_configurable_channel_token, 0, DEVICE_IOT_CONFIG_CHANNEL_TOKEN_MAX_SIZE);
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
bool DeviceIotServiceProvider::initService( void *arg ){

  if( nullptr != this->m_http_client ){
    this->m_http_client->SetClient(reinterpret_cast<iClientInterface*>(arg));
  }

  // __task_scheduler.setInterval( [&]() { this->handleDeviceIotConfigRequest(); }, HTTP_REQUEST_DURATION, __i_dvc_ctrl.millis_now() );
  this->m_device_config_request_cb_id = __task_scheduler.updateInterval(
    this->m_device_config_request_cb_id,
    [&]() {
      this->handleDeviceIotConfigRequest();
    },
    this->m_server_configurable_mqtt_keep_alive*MILLISECOND_DURATION_1000
  );

#if defined(ENABLE_MQTT_SERVICE)
  // clear all mqtt old configs
  __mqtt_general_table.clear();
  __mqtt_pubsub_table.clear();
  __mqtt_lwt_table.clear();
#endif

  return ServiceProvider::initService(arg);
}

/**
 * handle registration otp request
 */
void DeviceIotServiceProvider::handleRegistrationOtpRequest( device_iot_config_table *_device_iot_configs, pdiutil::string &_response ){

  pdiutil::string otpurl;

  if( nullptr != _device_iot_configs ){

    otpurl = _device_iot_configs->device_iot_host;
    otpurl += DEVICE_IOT_OTP_REQ_URL;

    size_t mac_index = otpurl.find("[mac]");
    if( pdiutil::string::npos != mac_index )
    {
      otpurl.replace( mac_index, 5, __i_dvc_ctrl.getDeviceMac().c_str() );
    }

    size_t duid_index = otpurl.find("[duid]");
    if( pdiutil::string::npos != duid_index )
    {
      otpurl.replace( duid_index, 6, _device_iot_configs->device_iot_duid );
    }
  }

  LogFmtI("Handling device otp Http Request : %s\n", otpurl.c_str());

  if( otpurl.size() > 5 && nullptr != this->m_http_client ){

    this->m_http_client->Begin();
    this->m_http_client->SetUserAgent("pdistack");
    this->m_http_client->SetBasicAuthorization("mac", __i_dvc_ctrl.getDeviceMac().c_str());
    this->m_http_client->SetTimeout(2*MILLISECOND_DURATION_1000);

    int _httpCode = this->m_http_client->Get(otpurl.c_str());
    
    char *http_resp = nullptr;
    int16_t httl_resp_len = 0;
    this->m_http_client->GetResponse( http_resp, httl_resp_len );

    LogFmtI("Http device otp Response code : %d\n", _httpCode );
    if ( _httpCode == HTTP_RESP_OK && nullptr != http_resp && httl_resp_len <= DEVICE_IOT_OTP_API_RESP_LENGTH ) {

      _response = http_resp;

      pdiutil::string::size_type _found_status = _response.find(DEVICE_IOT_OTP_STATUS_KEY);
      pdiutil::string::size_type _found_reconfig = _response.find(DEVICE_IOT_CONFIG_RECONFIGURE_KEY);
      pdiutil::string::size_type _found_otp = _response.find(DEVICE_IOT_OTP_KEY);
      if( _found_status != pdiutil::string::npos &&
          _found_reconfig != pdiutil::string::npos &&
          _found_otp != pdiutil::string::npos)
      {
        __task_scheduler.setTimeout( [&]() { __mqtt_service.stop(); }, 1, __i_dvc_ctrl.millis_now() );
      }
    }else{

      _response = "{\"status\":false,\"remark\":";
      _response += "\"Device request failed. ErrCode(";
      _response += pdiutil::to_string(_httpCode);
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

  pdiutil::string configurl = this->m_device_iot_configs.device_iot_host;
  bool valid_host = ( configurl.size() > 5 && pdiutil::string::npos != configurl.find("http") );
  configurl += DEVICE_IOT_CONFIG_REQ_URL;

  pdiutil::string::size_type mac_index = configurl.find("[mac]");
  if( pdiutil::string::npos != mac_index )
  {
    configurl.replace( mac_index, 5, __i_dvc_ctrl.getDeviceMac().c_str() );
  }

  pdiutil::string::size_type duid_index = configurl.find("[duid]");
  if( pdiutil::string::npos != duid_index )
  {
    configurl.replace( duid_index, 6, this->m_device_iot_configs.device_iot_duid );
  }

  LogFmtI("Handling device iot config Request : %s\n", configurl.c_str());

  if( valid_host && nullptr != this->m_http_client ){

    this->m_http_client->Begin();
    this->m_http_client->SetUserAgent("pdistack");
    this->m_http_client->SetBasicAuthorization("mac", __i_dvc_ctrl.getDeviceMac().c_str());
    this->m_http_client->SetTimeout(2*MILLISECOND_DURATION_1000);
    int _httpCode = this->m_http_client->Get(configurl.c_str());

    char *http_resp = nullptr;
    int16_t httl_resp_len = 0;
    this->m_http_client->GetResponse( http_resp, httl_resp_len );

    if ( _httpCode == HTTP_RESP_OK && nullptr != http_resp ) {

      if( httl_resp_len < DEVICE_IOT_CONFIG_RESP_MAX_SIZE ){

        if( 0 <= __strstr( http_resp, (char*)DEVICE_IOT_CONFIG_CHANNEL_TOKEN_KEY, DEVICE_IOT_CONFIG_RESP_MAX_SIZE - strlen(DEVICE_IOT_CONFIG_CHANNEL_TOKEN_KEY) ) ){

          bool _json_result = __get_from_json( http_resp, (char*)DEVICE_IOT_CONFIG_CHANNEL_TOKEN_KEY, this->m_server_configurable_channel_token, DEVICE_IOT_CONFIG_CHANNEL_TOKEN_MAX_SIZE-1 ) &&
            __get_from_json( http_resp, (char*)DEVICE_IOT_CONFIG_CHANNEL_WRITE_KEY, this->m_server_configurable_channel_write, DEVICE_IOT_CONFIG_CHANNEL_MAX_BUFF_SIZE-1 ) &&
            __get_from_json( http_resp, (char*)DEVICE_IOT_CONFIG_CHANNEL_READ_KEY, this->m_server_configurable_channel_read, DEVICE_IOT_CONFIG_CHANNEL_MAX_BUFF_SIZE-1 );
          
          if(  _json_result && strlen( this->m_server_configurable_channel_token ) && strlen( this->m_server_configurable_channel_write ) && strlen( this->m_server_configurable_channel_read ) ){

            LogFmtI("Got Token : %s\n", this->m_server_configurable_channel_token );
            LogFmtI("Got Write Channel : %s\n", this->m_server_configurable_channel_write );
            LogFmtI("Got Read Channel : %s\n", this->m_server_configurable_channel_read );

            this->handleServerConfigurableParameters( http_resp );

            this->m_mqtt_connection_check_cb_id = __task_scheduler.updateInterval(
              this->m_mqtt_connection_check_cb_id,
              [&]() {
                this->handleConnectivityCheck();
              },
              this->m_server_configurable_mqtt_keep_alive*MILLISECOND_DURATION_1000, DEFAULT_TASK_PRIORITY,
              ( __i_dvc_ctrl.millis_now() + MQTT_INITIALIZE_DURATION )
            );
#if defined(ENABLE_MQTT_SERVICE)
            __task_scheduler.setTimeout( [&]() { this->configureMQTT(); }, 1, __i_dvc_ctrl.millis_now() );
#endif
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

#if defined(ENABLE_MQTT_SERVICE)
  bool _is_mqtt_connected = __mqtt_service.m_mqtt_client.is_mqtt_connected();
  LogFmtI("Device iot mqtt connection check cycle : %d\n", (int)_is_mqtt_connected );
  if( !_is_mqtt_connected ) {
    this->m_token_validity = false;
    __mqtt_service.stop();
  }
#endif
}

#if defined(ENABLE_MQTT_SERVICE)

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

  memcpy( _mqtt_general_configs.host, this->m_server_configurable_channel_host, strlen(this->m_server_configurable_channel_host) );
  _mqtt_general_configs.port = this->m_server_configurable_channel_port;

  Http_Client::BuildBasicAuthorization("mac", __i_dvc_ctrl.getDeviceMac().c_str(), _mqtt_general_configs.client_id, MQTT_CLIENT_ID_BUF_SIZE);
  // strcpy( _mqtt_general_configs.client_id, this->m_device_iot_configs.device_iot_duid );
  strcpy( _mqtt_general_configs.username, this->m_device_iot_configs.device_iot_duid );
  memcpy( _mqtt_general_configs.password, this->m_server_configurable_channel_token, DEVICE_IOT_CONFIG_CHANNEL_TOKEN_MAX_SIZE );
  _mqtt_general_configs.keepalive = this->m_server_configurable_mqtt_keep_alive;
  _mqtt_general_configs.clean_session = 1;

  strcpy( _mqtt_pubsub_configs.publish_topics[0].topic, this->m_server_configurable_channel_write );
  strcpy( _mqtt_pubsub_configs.subscribe_topics[0].topic, this->m_server_configurable_channel_read );
  // _mqtt_pubsub_configs.publish_frequency = this->m_server_configurable_sensor_data_publish_freq; // let this device iot service manage the publish events

  strcpy( _mqtt_lwt_configs.will_topic, this->m_server_configurable_channel_read );
  strcpy( _mqtt_lwt_configs.will_message, RODT_ATTR("{\"duid\":\"[duid]\"}") );
  _mqtt_lwt_configs.will_qos = 1;
  _mqtt_lwt_configs.will_retain = 0;
  __find_and_replace( _mqtt_lwt_configs.will_message, "[duid]", this->m_device_iot_configs.device_iot_duid, 2 );

  __database_service.set_mqtt_general_config_table( &_mqtt_general_configs );
  __database_service.set_mqtt_lwt_config_table( &_mqtt_lwt_configs );
  __database_service.set_mqtt_pubsub_config_table( &_mqtt_pubsub_configs );

  __mqtt_service.setMqttSubscribeDataCallback(DeviceIotServiceProvider::handleSubscribeCallback);

  __task_scheduler.setTimeout( [&]() { __mqtt_service.handleMqttConfigChange(); }, 1, __i_dvc_ctrl.millis_now() );
}

/**
 * handle mqtt subscribe data callback
 */
void DeviceIotServiceProvider::handleSubscribeCallback( uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len ){

  char *topicBuf = new char[topic_len+1], *dataBuf = new char[data_len+1];

  memcpy(topicBuf, topic, topic_len);
  topicBuf[topic_len] = 0;

  memcpy(dataBuf, data, data_len);
  dataBuf[data_len] = 0;

  pdiutil::vector<pdiutil::string> allowed_interface_list = __device_iot_service.m_server_configurable_interface_read;
  allowed_interface_list.insert(allowed_interface_list.end(), __device_iot_service.m_server_configurable_interface_write.begin(), __device_iot_service.m_server_configurable_interface_write.end());
  
  #if defined( ENABLE_GPIO_SERVICE )
  __gpio_service.applyGpioJsonPayload( dataBuf, data_len, &allowed_interface_list );
  #endif

  #if defined(ENABLE_SERIAL_SERVICE)
  __serial_service.applySerialJsonPayload( dataBuf, data_len, &allowed_interface_list );
  #endif

  // handle channel write action as soon as possible to reflect applied json payload
  __device_iot_service.m_handle_channel_write_asap = true;
  __task_scheduler.setTimeout( [&]() { __device_iot_service.handleSensorData(); }, 1, __i_dvc_ctrl.millis_now() );

  // handle reconfiguration request
  char *_value_buff = new char[50];
  memset( _value_buff, 0, 50 );
  bool _json_result = __get_from_json( dataBuf, (char*)DEVICE_IOT_CONFIG_RECONFIGURE_KEY, _value_buff, 6 );
  uint16_t reconfigure = StringToUint16( _value_buff, 6 );
  if( _json_result && reconfigure == 1 ){
    LogI("Reconfiguring...\n");
    __task_scheduler.setTimeout( [&]() { __mqtt_service.stop(); }, 1, __i_dvc_ctrl.millis_now() );
  }

  delete[] topicBuf; delete[] dataBuf; delete[] _value_buff;
}

#endif

/**
 * handle mqtt config parameters from server
 */
void DeviceIotServiceProvider::handleServerConfigurableParameters(char* json_resp){

  char *_value_buff = new char[100];

  memset( _value_buff, 0, 100 );
  bool _json_result = __get_from_json( json_resp, (char*)DEVICE_IOT_CONFIG_DEVICEID_KEY, _value_buff, 31 );
  uint64_t device_id = StringToUint64( _value_buff, 31 );
  if( _json_result && 0 < device_id && device_id <= UINT64_MAX ){

    this->m_server_configurable_device_id = device_id;
    LogFmtI("Got Device ID : %d\n", (int)this->m_server_configurable_device_id);
  }

  memset( this->m_server_configurable_channel_host, 0, DEVICE_IOT_CONFIG_CHANNEL_MAX_BUFF_SIZE );
  _json_result = __get_from_json( json_resp, (char*)DEVICE_IOT_CONFIG_CHANNEL_HOST_KEY, this->m_server_configurable_channel_host, DEVICE_IOT_CONFIG_CHANNEL_MAX_BUFF_SIZE-1 );
  if( _json_result && strlen(this->m_server_configurable_channel_host) > 5 ){

    LogFmtI("Got Channel Host : %s\n", this->m_server_configurable_channel_host);
  }else{

    // else parse the host from iot host config
    http_req_t httpreq; httpreq.init(this->m_device_iot_configs.device_iot_host);
    memcpy( this->m_server_configurable_channel_host, httpreq.host, pdistd::min((int)strlen( httpreq.host ), (int)(DEVICE_IOT_CONFIG_CHANNEL_MAX_BUFF_SIZE-1)) );
    LogFmtW("Using Iot Channel Host : %s\n", this->m_server_configurable_channel_host);
  }

  memset( _value_buff, 0, 100 );
  _json_result = __get_from_json( json_resp, (char*)DEVICE_IOT_CONFIG_CHANNEL_PORT_KEY, _value_buff, 31 );
  if( _json_result && 0 < this->m_server_configurable_channel_port && this->m_server_configurable_channel_port <= UINT32_MAX ){

    this->m_server_configurable_channel_port = StringToUint32( _value_buff, 31 );
    LogFmtI("Got Channel Port : %d\n", (int)this->m_server_configurable_channel_port);
  }else{
    this->m_server_configurable_channel_port = DEVICE_IOT_DEFAULT_CHANNEL_DATA_PORT;
  }

  memset( _value_buff, 0, 100 );
  _json_result = __get_from_json( json_resp, (char*)DEVICE_IOT_CONFIG_DATA_RATE_KEY, _value_buff, 6 );
  uint16_t data_rate = StringToUint16( _value_buff, 6 );
  if( _json_result && SENSOR_DATA_PUBLISH_FREQ_MIN_LIMIT <= data_rate && data_rate <= SENSOR_DATA_PUBLISH_FREQ_MAX_LIMIT ){

    this->m_server_configurable_sensor_data_publish_freq = data_rate;
    LogFmtI("Got Data rate : %d\n", data_rate);
  }

  uint16_t sample_rate = round ( this->m_server_configurable_sensor_data_publish_freq / ( ( SENSOR_DATA_SAMPLES_PER_PUBLISH_MAX_LIMIT * 0.125 ) * ( log(this->m_server_configurable_sensor_data_publish_freq) ) ) );
  if( 0 < sample_rate && sample_rate <= SENSOR_DATA_SAMPLES_PER_PUBLISH_MAX_LIMIT ){

    this->m_server_configurable_sample_per_publish = sample_rate;
    LogFmtI("Got Sample rate : %d\n", sample_rate);
  }

  memset( _value_buff, 0, 100 );
  _json_result = __get_from_json( json_resp, (char*)DEVICE_IOT_CONFIG_MQTT_KEEP_ALIVE_KEY, _value_buff, 6 );
  uint16_t keep_alive = StringToUint16( _value_buff, 6 );
  if( _json_result && DEVICE_IOT_MQTT_KEEP_ALIVE_MIN <= keep_alive && keep_alive <= DEVICE_IOT_MQTT_KEEP_ALIVE_MAX ){

    this->m_server_configurable_mqtt_keep_alive = keep_alive;
    LogFmtI("Got keep alive : %d\n", keep_alive);
  }

  memset( _value_buff, 0, 100 );
  _json_result = __get_from_json( json_resp, (char*)DEVICE_IOT_CONFIG_INTERFACE_READ_KEY, _value_buff, 99 );
  this->m_server_configurable_interface_read.clear();
  if( _json_result && strlen(_value_buff) > 0 ){

    uint16_t lastcommaindex = 0, i = 0;
    for (i = 0; i < strlen(_value_buff); i++){
      
      if( _value_buff[i] == ',' ){

        this->m_server_configurable_interface_read.push_back( pdiutil::string( _value_buff + lastcommaindex, i - lastcommaindex ) );
        lastcommaindex = i+1;
      }     
    }
    this->m_server_configurable_interface_read.push_back( pdiutil::string( _value_buff + lastcommaindex, i - lastcommaindex ) );

    LogFmtI("Got Read Interface : %s\n", _value_buff);

    for(uint16_t i = 0; i < this->m_server_configurable_interface_read.size(); i++ ){
      
      int16_t imode = -1;

      if( __are_arrays_equal(SERIAL_INTERFACE_UART, this->m_server_configurable_interface_read[i].c_str(), strlen(SERIAL_INTERFACE_UART)) ){

        imode = SERIAL_READ;
      }else if( __are_arrays_equal(SERIAL_INTERFACE_CAN, this->m_server_configurable_interface_read[i].c_str(), strlen(SERIAL_INTERFACE_CAN)) ){

      }else if( __are_arrays_equal(SERIAL_INTERFACE_I2C, this->m_server_configurable_interface_read[i].c_str(), strlen(SERIAL_INTERFACE_I2C)) ){

      }else if( __are_arrays_equal(SERIAL_INTERFACE_SPI, this->m_server_configurable_interface_read[i].c_str(), strlen(SERIAL_INTERFACE_SPI)) ){

      }else{

        #if defined( ENABLE_GPIO_SERVICE )
        bool isDigital = this->m_server_configurable_interface_read[i][0] == 'D' || this->m_server_configurable_interface_read[i][0] == 'd';
        imode = isDigital ? DIGITAL_READ : ANALOG_READ;
        #endif
      }

      pdiutil::string payloadtoapply = "{\"data\":{\""; 
      payloadtoapply += this->m_server_configurable_interface_read[i];
      payloadtoapply += "\":{\"mode\":";
      payloadtoapply += pdiutil::to_string(imode);
      payloadtoapply += ",\"val\":\"NA\"}}}";

      if( imode == DIGITAL_READ || imode == ANALOG_READ ){

        #if defined( ENABLE_GPIO_SERVICE )
        __gpio_service.applyGpioJsonPayload((char*)payloadtoapply.c_str(), payloadtoapply.size(), &this->m_server_configurable_interface_read);
        #endif
      }
    }
  }

  memset( _value_buff, 0, 100 );
  _json_result = __get_from_json( json_resp, (char*)DEVICE_IOT_CONFIG_INTERFACE_WRITE_KEY, _value_buff, 99 );
  this->m_server_configurable_interface_write.clear();
  if( _json_result && strlen(_value_buff) > 0 ){

    uint16_t lastcommaindex = 0, i = 0;
    for (i = 0; i < strlen(_value_buff); i++){
      
      if( _value_buff[i] == ',' ){

        this->m_server_configurable_interface_write.push_back( pdiutil::string( _value_buff + lastcommaindex, i - lastcommaindex ) );
        lastcommaindex = i+1;
      }     
    }
    this->m_server_configurable_interface_write.push_back( pdiutil::string( _value_buff + lastcommaindex, i - lastcommaindex ) );

    LogFmtI("Got Write Interface : %s\n", _value_buff);

    for(uint16_t i = 0; i < this->m_server_configurable_interface_write.size(); i++ ){

      int16_t imode = -1;

      if( __are_arrays_equal(SERIAL_INTERFACE_UART, this->m_server_configurable_interface_write[i].c_str(), strlen(SERIAL_INTERFACE_UART)) ){

        imode = SERIAL_WRITE;
      }else if( __are_arrays_equal(SERIAL_INTERFACE_CAN, this->m_server_configurable_interface_write[i].c_str(), strlen(SERIAL_INTERFACE_CAN)) ){

      }else if( __are_arrays_equal(SERIAL_INTERFACE_I2C, this->m_server_configurable_interface_write[i].c_str(), strlen(SERIAL_INTERFACE_I2C)) ){

      }else if( __are_arrays_equal(SERIAL_INTERFACE_SPI, this->m_server_configurable_interface_write[i].c_str(), strlen(SERIAL_INTERFACE_SPI)) ){

      }else{

        #if defined( ENABLE_GPIO_SERVICE )
        bool isDigital = this->m_server_configurable_interface_write[i][0] == 'D' || this->m_server_configurable_interface_write[i][0] == 'd';
        // Using Ananlog write PWM on digital pins
        if( !isDigital ){
          this->m_server_configurable_interface_write[i][0] = 'D';
        }

        imode = isDigital ? DIGITAL_WRITE : ANALOG_WRITE;
        #endif
      }

      pdiutil::string payloadtoapply = "{\"data\":{\""; 
      payloadtoapply += this->m_server_configurable_interface_write[i];
      payloadtoapply += "\":{\"mode\":";
      payloadtoapply += pdiutil::to_string(imode);
      payloadtoapply += ",\"val\":\"NA\"}}}";

      if( imode == DIGITAL_WRITE || imode == ANALOG_WRITE ){

        #if defined( ENABLE_GPIO_SERVICE )
        __gpio_service.applyGpioJsonPayload((char*)payloadtoapply.c_str(), payloadtoapply.size(), &this->m_server_configurable_interface_write);
        #endif
      }
    }
  }

  memset( _value_buff, 0, 100 );
  _json_result = __get_from_json( json_resp, (char*)DEVICE_IOT_CONFIG_INTERFACE_EVENT_KEY, _value_buff, 99 );
  if( _json_result && strlen(_value_buff) > 0 ){

    pdiutil::vector<pdiutil::string> allowed_interface_list = this->m_server_configurable_interface_read;
    allowed_interface_list.insert(allowed_interface_list.end(), this->m_server_configurable_interface_write.begin(), this->m_server_configurable_interface_write.end());

    #if defined( ENABLE_GPIO_SERVICE )
    __gpio_service.setDeviceId(this->m_device_iot_configs.device_iot_duid);
    __gpio_service.setHttpHost(this->m_device_iot_configs.device_iot_host);
    __gpio_service.m_gpio_config_copy.clearAllGpioEvents();
    __gpio_service.applyGpioEventJsonPayload(_value_buff, strlen(_value_buff), &allowed_interface_list);
    #endif
  }

  this->beginSensorData();

  this->m_device_config_request_cb_id = __task_scheduler.updateInterval(
    this->m_device_config_request_cb_id,
    [&]() {
      this->handleDeviceIotConfigRequest();
    },
    this->m_server_configurable_mqtt_keep_alive*MILLISECOND_DURATION_1000,
    DEFAULT_TASK_PRIORITY,
    ( __i_dvc_ctrl.millis_now() + MQTT_INITIALIZE_DURATION)
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
    (((float)this->m_server_configurable_sensor_data_publish_freq/(float)this->m_server_configurable_sample_per_publish)*1000.0)
  );
  this->m_sample_index = 0;
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

  LogFmtI("Handling sensor data samples: %d\n", this->m_sample_index);

  if( nullptr == this->m_device_iot ){
    return;
  }

  this->m_device_iot->sampleHook();

  static uint64_t lastpublishtimestamp = __i_dvc_ctrl.millis_now();
  bool istimetopublish = (__i_dvc_ctrl.millis_now() - lastpublishtimestamp) >= (this->m_server_configurable_sensor_data_publish_freq*1000);

  if( (this->m_sample_index >= this->m_server_configurable_sample_per_publish-1) || this->m_handle_channel_write_asap || istimetopublish ){

    this->m_sample_index = 0;
    this->m_handle_channel_write_asap = false;
    lastpublishtimestamp = __i_dvc_ctrl.millis_now();

    pdiutil::string _payload = "{\"id\":[did],\"packet_type\":\"data\",\"packet_version\":\"";
    _payload += DEVICE_IOT_PACKET_VERSION;
    _payload += "\",\"payload\":";
    this->m_device_iot->dataHook(_payload);
    _payload += "}";

    size_t duid_index = _payload.find("[duid]");
    if( pdiutil::string::npos != duid_index )
    {
      _payload.replace( duid_index, 6, this->m_device_iot_configs.device_iot_duid );
    }

    size_t did_index = _payload.find("[did]");
    if( pdiutil::string::npos != did_index )
    {
      _payload.replace( did_index, 5, pdiutil::to_string(this->m_server_configurable_device_id).c_str() );
    }

    if( this->m_server_configurable_interface_read.size() > 0 || this->m_server_configurable_interface_write.size() > 0 ){

#if defined(ENABLE_MQTT_SERVICE)
      __task_scheduler.setTimeout( [&]() { __mqtt_service.handleMqttPublish(true); }, 1, __i_dvc_ctrl.millis_now(), DEFAULT_TASK_PRIORITY+1 );
      __task_scheduler.rebaseAndRestartPrioTasks();

      memset( __mqtt_service.m_mqtt_payload, 0, MQTT_PAYLOAD_BUF_SIZE );
      if( _payload.size()+1 < MQTT_PAYLOAD_BUF_SIZE ){
        // _payload.toCharArray( __mqtt_service.m_mqtt_payload, _payload.size()+1);
        strncpy(__mqtt_service.m_mqtt_payload, _payload.c_str(), _payload.size()); 
      }else{
        strcat( __mqtt_service.m_mqtt_payload, RODT_ATTR("mqtt data is too big to fit in buffer !"));
      }
#endif
    }else{

      // Considering server not provided any interface to operate on. So closing the current mqtt 
      // and will retry soon to get Updated config from server
      __task_scheduler.setTimeout( [&]() { __mqtt_service.stop(); }, 1, __i_dvc_ctrl.millis_now() );
    }
  }else{
    this->m_sample_index++;
  }
}

/**
 * print Device reg configs to terminal
 */
void DeviceIotServiceProvider::printConfigToTerminal(iTerminalInterface *terminal)
{
  if( nullptr != terminal ){

    device_iot_config_table _device_iot_configs;
    __database_service.get_device_iot_config_table(&_device_iot_configs);

    terminal->writeln();
    terminal->writeln_ro(RODT_ATTR("Device IOT Configs :"));
    terminal->writeln(_device_iot_configs.device_iot_host);
  }
}

DeviceIotServiceProvider __device_iot_service;

#endif
