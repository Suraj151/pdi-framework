/****************************** Gpio service **********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include <config/Config.h>

#if defined(ENABLE_GPIO_SERVICE)


#include "GpioServiceProvider.h"


__gpio_alert_track_t __gpio_alert_track = {
  false, 0, -1
};

/**
 * GpioServiceProvider constructor.
 */
GpioServiceProvider::GpioServiceProvider():
  m_gpio_http_request_cb_id(0),
  m_update_gpio_table_from_copy(true),
  m_http_client(Http_Client::GetStaticInstance())
{
  for (size_t i = 0; i < MAX_DIGITAL_GPIO_PINS; i++) {
    this->m_digital_blinker[i] = nullptr;
  }
}

/**
 * GpioServiceProvider destructor
 */
GpioServiceProvider::~GpioServiceProvider(){

  this->m_http_client = nullptr;

  for (size_t i = 0; i < MAX_DIGITAL_GPIO_PINS; i++) {
    if( nullptr != this->m_digital_blinker[i] ){
      __i_dvc_ctrl.releaseGpioBlinkerInstance(this->m_digital_blinker[i]);
      this->m_digital_blinker[i] = nullptr;
    }
  }
}

/**
 * start gpio services if enabled
 */
void GpioServiceProvider::begin( iClientInterface* _client ){

  if( nullptr == _client ){
    return;
  }

  if( nullptr != this->m_http_client ){
    this->m_http_client->SetClient(_client);
  }

  this->handleGpioModes();
  __database_service.get_gpio_config_table(&this->m_gpio_config_copy);

  __task_scheduler.setInterval( [&]() { this->handleGpioOperations(); }, GPIO_OPERATION_DURATION, __i_dvc_ctrl.millis_now() );
  __task_scheduler.setInterval( [&]() { this->enable_update_gpio_table_from_copy(); }, GPIO_TABLE_UPDATE_DURATION, __i_dvc_ctrl.millis_now() );
}

/**
 * post gpio data to server specified in gpio configs
 */
bool GpioServiceProvider::handleGpioHttpRequest( bool isAlertPost ){

  bool status = false;

  std::string posturl = this->m_gpio_config_copy.gpio_host;

  if( isAlertPost ){
    posturl += GPIO_ALERT_POST_HTTP_URL;
  }else{
    posturl += GPIO_DATA_POST_HTTP_URL;
  }

  LogI("Handling GPIO Http Request\n");

  if( posturl.size() > 5 && this->m_gpio_config_copy.gpio_port > 0 &&
    ( isAlertPost ? true : ( this->m_gpio_config_copy.gpio_post_frequency > 0 ) ) &&
    nullptr != this->m_http_client
  ){

    std::string *_payload = new std::string();

    if( nullptr != _payload ){

      this->appendGpioJsonPayload( *_payload, isAlertPost );

      this->m_http_client->Begin();

      #ifdef ENABLE_DEVICE_IOT
      this->m_http_client->SetUserAgent("esp");
      this->m_http_client->SetBasicAuthorization("iot-otp", __i_dvc_ctrl.getDeviceMac().c_str());
      #else
      this->m_http_client->SetUserAgent("Ewings");
      this->m_http_client->SetBasicAuthorization("user", "password");
      #endif

      this->m_http_client->AddReqHeader(HTTP_HEADER_KEY_CONTENT_TYPE, "application/json");
      this->m_http_client->SetTimeout(2*MILLISECOND_DURATION_1000);

      LogFmtI("posting data : %s\n", _payload->c_str());

      status = ( HTTP_RESP_OK == this->m_http_client->Post( posturl.c_str(), _payload->c_str() ) );
      
      this->m_http_client->End(true);
      delete _payload;
    }
  }else{
    LogI("GPIO Http Request not initializing or failed or Not Configured Correctly\n");
  }

  return status;
}

/**
 * append gpio payload to string arg
 *
 * @param std::string& _payload
 */
void GpioServiceProvider::appendGpioJsonPayload( std::string &_payload, bool isAlertPost ){

  _payload += "{";

  if( __i_dvc_ctrl.getDeviceMac().size() ){

    _payload += "\"";
    _payload += GPIO_PAYLOAD_MAC_KEY;
    _payload += "\":\"";
    _payload += __i_dvc_ctrl.getDeviceMac().c_str();
    _payload += "\",";
  }

  if( isAlertPost ){

    _payload += "\"";
    _payload += GPIO_ALERT_PIN_KEY;
    _payload += "\":\"";

    if( __gpio_alert_track.alert_gpio_pin < MAX_DIGITAL_GPIO_PINS ){
      _payload += "D";
      _payload += std::to_string(__gpio_alert_track.alert_gpio_pin);
    }else{
      _payload += "A";
      _payload += std::to_string(__gpio_alert_track.alert_gpio_pin - MAX_DIGITAL_GPIO_PINS);
    }

    _payload += "\",";
  }

  _payload += "\"";
  _payload += GPIO_PAYLOAD_DATA_KEY;
  _payload += "\":{";

  for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS; _pin++) {

    if( !__i_dvc_ctrl.isExceptionalGpio(_pin) ){

      _payload += "\"D";
      _payload += std::to_string(_pin);
      _payload += "\":{\"";
      _payload += GPIO_PAYLOAD_MODE_KEY;
      _payload += "\":";
      _payload += std::to_string(this->m_gpio_config_copy.gpio_mode[_pin]);
      _payload += ",\"";
      _payload += GPIO_PAYLOAD_VALUE_KEY;
      _payload += "\":";
      _payload += std::to_string(this->m_gpio_config_copy.gpio_readings[_pin]);
      _payload += "},";
    }
  }

  for (uint8_t _pin = 0; _pin < MAX_ANALOG_GPIO_PINS; _pin++) {

    _payload += "\"A";
    _payload += std::to_string(_pin);
    _payload += "\":{\"";
    _payload += GPIO_PAYLOAD_MODE_KEY;
    _payload += "\":";
    _payload += std::to_string(this->m_gpio_config_copy.gpio_mode[MAX_DIGITAL_GPIO_PINS+_pin]);
    _payload += ",\"";
    _payload += GPIO_PAYLOAD_VALUE_KEY;
    _payload += "\":";
    _payload += std::to_string(this->m_gpio_config_copy.gpio_readings[MAX_DIGITAL_GPIO_PINS+_pin]);
    _payload += "}";

    if( ( MAX_ANALOG_GPIO_PINS - _pin ) > 1 ){
      _payload += ",";
    }
  }

  _payload += "}}";
}

/**
 * apply json payload to gpio operations
 *
 * @param char* _payload
 */
void GpioServiceProvider::applyGpioJsonPayload( char* _payload, uint16_t _payload_length ){

  LogFmtI("Applying GPIO from Json Payload : %s\n", _payload);

  if(
    0 <= __strstr( _payload, (char*)GPIO_PAYLOAD_DATA_KEY, _payload_length - strlen(GPIO_PAYLOAD_DATA_KEY) ) &&
    0 <= __strstr( _payload, (char*)GPIO_PAYLOAD_MODE_KEY, _payload_length - strlen(GPIO_PAYLOAD_MODE_KEY) ) &&
    0 <= __strstr( _payload, (char*)GPIO_PAYLOAD_VALUE_KEY, _payload_length - strlen(GPIO_PAYLOAD_VALUE_KEY) )
  ){

    int _pin_data_max_len = 30, _pin_values_max_len = 6;
    char _pin_label[_pin_values_max_len]; memset( _pin_label, 0, _pin_values_max_len); _pin_label[0] = 'D';
    char _pin_data[_pin_data_max_len], _pin_mode[_pin_values_max_len], _pin_value[_pin_values_max_len];
    // Decode the pin mode and value
    for (uint8_t _pin = 0; _pin < (MAX_DIGITAL_GPIO_PINS + MAX_ANALOG_GPIO_PINS); _pin++) {

      uint8_t _pin_label_n = _pin;

      if( _pin < MAX_DIGITAL_GPIO_PINS ){

      }else{
        _pin_label_n = _pin - MAX_DIGITAL_GPIO_PINS;
        _pin_label[0] = 'A';
      }

      _pin_label[1] = ( 0x30 + _pin_label_n ); memset( _pin_data, 0, _pin_data_max_len);
      memset( _pin_mode, 0, _pin_values_max_len); memset( _pin_value, 0, _pin_values_max_len);
      if( !__i_dvc_ctrl.isExceptionalGpio(_pin) && __get_from_json( _payload, _pin_label, _pin_data, _pin_data_max_len ) ){

        if( __get_from_json( _pin_data, (char*)GPIO_PAYLOAD_MODE_KEY, _pin_mode, _pin_values_max_len ) ){

          if( __get_from_json( _pin_data, (char*)GPIO_PAYLOAD_VALUE_KEY, _pin_value, _pin_values_max_len ) ){

            uint8_t _mode = StringToUint8( _pin_mode, _pin_values_max_len );
            uint16_t _value = StringToUint16( _pin_value, _pin_values_max_len );
            uint16_t _value_limit = _mode == ANALOG_WRITE ? ANALOG_GPIO_RESOLUTION : _mode == DIGITAL_BLINK ? _value+1 : GPIO_STATE_MAX;
            this->m_gpio_config_copy.gpio_mode[_pin] = _mode < GPIO_MODE_MAX ? _mode : this->m_gpio_config_copy.gpio_mode[_pin];
            this->m_gpio_config_copy.gpio_readings[_pin] = _value < _value_limit ? _value : this->m_gpio_config_copy.gpio_readings[_pin];
          }
        }
      }
    }
  }

}


#ifdef ENABLE_EMAIL_SERVICE
/**
 * handle gpio email alert
 * @return bool
 */
bool GpioServiceProvider::handleGpioEmailAlert(){

  bool status = false;

  LogI("Handling GPIO email alert\n");

  std::string *_payload = new std::string();

  if( nullptr != _payload ){

    this->appendGpioJsonPayload( *_payload, true );

    *_payload += "\n\nRegards\n";

    if( __i_dvc_ctrl.getDeviceMac().size() ){

      *_payload += __i_dvc_ctrl.getDeviceMac().c_str();
    }

    status = __email_service.sendMail( *_payload );

    delete _payload;
  }

  return status;
}
#endif

/**
 * handle gpio operations as per gpio configs
 */
void GpioServiceProvider::handleGpioOperations(){

  for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS; _pin++) {

    if( DIGITAL_BLINK != this->m_gpio_config_copy.gpio_mode[_pin] ){

      if( _pin < MAX_DIGITAL_GPIO_PINS && nullptr != this->m_digital_blinker[_pin] ){
        __i_dvc_ctrl.releaseGpioBlinkerInstance(this->m_digital_blinker[_pin]);
        this->m_digital_blinker[_pin] = nullptr;
      }
    }

    switch ( __i_dvc_ctrl.isExceptionalGpio(_pin) ? OFF : this->m_gpio_config_copy.gpio_mode[_pin] ) {

      default:
      case OFF:{
        this->m_gpio_config_copy.gpio_readings[_pin] = LOW;
        break;
      }
      case DIGITAL_WRITE:{
        __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, __i_dvc_ctrl.gpioFromPinMap( _pin ), this->m_gpio_config_copy.gpio_readings[_pin] );
        break;
      }
      case DIGITAL_READ:{
        this->m_gpio_config_copy.gpio_readings[_pin] = __i_dvc_ctrl.gpioRead(DIGITAL_READ, __i_dvc_ctrl.gpioFromPinMap( _pin ) );
        break;
      }
      case DIGITAL_BLINK:{
        if( nullptr != this->m_digital_blinker[_pin] ){

          this->m_digital_blinker[_pin]->updateConfig( __i_dvc_ctrl.gpioFromPinMap( _pin ), this->m_gpio_config_copy.gpio_readings[_pin] );
          this->m_digital_blinker[_pin]->start();
        }else{

          this->m_digital_blinker[_pin] = __i_dvc_ctrl.createGpioBlinkerInstance( __i_dvc_ctrl.gpioFromPinMap( _pin ), this->m_gpio_config_copy.gpio_readings[_pin] );
        }
        break;
      }
      case ANALOG_WRITE:{
        __i_dvc_ctrl.gpioWrite(ANALOG_WRITE, __i_dvc_ctrl.gpioFromPinMap( _pin ), this->m_gpio_config_copy.gpio_readings[_pin] );
        break;
      }
      case ANALOG_READ:{
        if( MAX_DIGITAL_GPIO_PINS <= _pin  ){
          this->m_gpio_config_copy.gpio_readings[_pin] = __i_dvc_ctrl.gpioRead(ANALOG_READ, A0 );
        }
        break;
      }
    }

    if( !__i_dvc_ctrl.isExceptionalGpio(_pin) && this->m_gpio_config_copy.gpio_alert_channel[_pin] != NO_ALERT ){

      bool _is_alert_condition = false;

      switch ( this->m_gpio_config_copy.gpio_alert_comparator[_pin] ) {

        case EQUAL:{
          _is_alert_condition = ( this->m_gpio_config_copy.gpio_readings[_pin] == this->m_gpio_config_copy.gpio_alert_values[_pin] );
          break;
        }
        case GREATER_THAN:{
          _is_alert_condition = ( this->m_gpio_config_copy.gpio_readings[_pin] > this->m_gpio_config_copy.gpio_alert_values[_pin] );
          break;
        }
        case LESS_THAN:{
          _is_alert_condition = ( this->m_gpio_config_copy.gpio_readings[_pin] < this->m_gpio_config_copy.gpio_alert_values[_pin] );
          break;
        }
        default: break;
      }

      uint32_t _now = __i_dvc_ctrl.millis_now();
      if( _is_alert_condition && ( __gpio_alert_track.is_last_alert_succeed ?
        GPIO_ALERT_DURATION_FOR_SUCCEED < ( _now - __gpio_alert_track.last_alert_millis ) :
        GPIO_ALERT_DURATION_FOR_FAILED < ( _now - __gpio_alert_track.last_alert_millis )
      ) ){

        __gpio_alert_track.alert_gpio_pin = _pin;
        __gpio_alert_track.last_alert_millis = _now;
        switch ( this->m_gpio_config_copy.gpio_alert_channel[_pin] ) {

          #ifdef ENABLE_EMAIL_SERVICE
          case EMAIL:{
            __gpio_alert_track.is_last_alert_succeed = this->handleGpioEmailAlert();
            break;
          }
          #endif
          case HTTP_SERVER:{
            __gpio_alert_track.is_last_alert_succeed = this->handleGpioHttpRequest(true);
            break;
          }
          case NO_ALERT:
          default: break;
        }
      }
    }

  }

  if( this->m_update_gpio_table_from_copy ){
    __database_service.set_gpio_config_table(&this->m_gpio_config_copy);
    this->m_update_gpio_table_from_copy = false;
  }

}

/**
 * enable gpio data update to database from virtual table
 */
void GpioServiceProvider::enable_update_gpio_table_from_copy(){
  this->m_update_gpio_table_from_copy = true;
}

/**
 * handle gpio modes for their operations.
 */
void GpioServiceProvider::handleGpioModes( int _gpio_config_type ){

  gpio_config_table _gpio_configs;
  __database_service.get_gpio_config_table(&_gpio_configs);

  for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS; _pin++) {

    __i_dvc_ctrl.gpioMode((GPIO_MODE)_gpio_configs.gpio_mode[_pin], __i_dvc_ctrl.gpioFromPinMap( _pin ));
  }

  this->m_gpio_config_copy = _gpio_configs;

  if( strlen( this->m_gpio_config_copy.gpio_host ) > 5 && this->m_gpio_config_copy.gpio_port > 0 &&
    this->m_gpio_config_copy.gpio_post_frequency > 0
  ){
    this->m_gpio_http_request_cb_id = __task_scheduler.updateInterval(
      this->m_gpio_http_request_cb_id,
      [&]() { this->handleGpioHttpRequest(); },
      this->m_gpio_config_copy.gpio_post_frequency*MILLISECOND_DURATION_1000
    );
  }else{
    __task_scheduler.clearInterval( this->m_gpio_http_request_cb_id );
    this->m_gpio_http_request_cb_id = 0;
  }

}

/**
 * print gpio configs
 */
void GpioServiceProvider::printGpioConfigLogs(){


  LogI("\nGPIO Configs (mode) :\n");
  for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS; _pin++) {
    LogFmtI("%d\t", this->m_gpio_config_copy.gpio_mode[_pin]);
  }

  LogI("\nGPIO Configs (readings) :\n");
  for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS; _pin++) {
    LogFmtI("%d\t", this->m_gpio_config_copy.gpio_readings[_pin]);
  }

  LogI("\nGPIO Configs (alert comparator) :\n");
  for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS; _pin++) {
    LogFmtI("%d\t", this->m_gpio_config_copy.gpio_alert_comparator[_pin]);
  }

  LogI("\nGPIO Configs (alert channels) :\n");
  for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS; _pin++) {
    LogFmtI("%d\t", this->m_gpio_config_copy.gpio_alert_channel[_pin]);
  }

  LogI("\nGPIO Configs (alert values) :\n");
  for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS+MAX_ANALOG_GPIO_PINS; _pin++) {
    LogFmtI("%d\t", this->m_gpio_config_copy.gpio_alert_values[_pin]);
  }

  LogI("\nGPIO Configs (server) :\n");
  LogFmtI("%s\t%d\t%d\n\n", this->m_gpio_config_copy.gpio_host, this->m_gpio_config_copy.gpio_port, 
  this->m_gpio_config_copy.gpio_post_frequency);
}


GpioServiceProvider __gpio_service;

#endif
