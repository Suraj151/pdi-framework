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
  m_wifi_client(nullptr),
  m_wifi(nullptr)
{
  for (size_t i = 0; i < MAX_DIGITAL_GPIO_PINS; i++) {
    this->m_digital_blinker[i] = nullptr;
  }
}

/**
 * GpioServiceProvider destructor
 */
GpioServiceProvider::~GpioServiceProvider(){

  this->m_wifi = nullptr;
  this->m_wifi_client = nullptr;

  for (size_t i = 0; i < MAX_DIGITAL_GPIO_PINS; i++) {
    if( nullptr != this->m_digital_blinker[i] ){
      delete this->m_digital_blinker[i];
      this->m_digital_blinker[i] = nullptr;
    }
  }
}

/**
 * start gpio services if enabled
 */
void GpioServiceProvider::begin( iWiFiInterface* _wifi, iWiFiClientInterface* _wifi_client ){

  if( nullptr == _wifi || nullptr == _wifi_client ){
    return;
  }

  this->m_wifi = _wifi;
  this->m_wifi_client = _wifi_client;

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

  memset( __http_service.m_host, 0, HTTP_HOST_ADDR_MAX_SIZE );
  strcpy( __http_service.m_host, this->m_gpio_config_copy.gpio_host );

  if( isAlertPost ){
    strcat( __http_service.m_host, GPIO_ALERT_POST_HTTP_URL );
  }else{
    strcat( __http_service.m_host, GPIO_DATA_POST_HTTP_URL );
  }

  __http_service.m_port = this->m_gpio_config_copy.gpio_port;

  LogI("Handling GPIO Http Request\n");

  if( strlen( __http_service.m_host ) > 5 &&
    __http_service.m_port > 0 &&
    ( isAlertPost ? true : ( this->m_gpio_config_copy.gpio_post_frequency > 0 ) ) &&
    nullptr != this->m_wifi_client &&
    __http_service.m_http_client->begin( *this->m_wifi_client, __http_service.m_host )
  ){

    String *_payload = new String("");

    if( nullptr != _payload ){

      this->appendGpioJsonPayload( *_payload, isAlertPost );

      #ifdef ENABLE_DEVICE_IOT

      uint8_t mac[6];
      char macStr[18] = { 0 };
      wifi_get_macaddr(STATION_IF, mac);
      sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

      __http_service.m_http_client->setUserAgent("esp");
      __http_service.m_http_client->setAuthorization("iot-otp", macStr);
      __http_service.m_http_client->setTimeout(3*MILLISECOND_DURATION_1000);
      #else
      __http_service.m_http_client->setUserAgent("Ewings");
      __http_service.m_http_client->setAuthorization("user", "password");
      __http_service.m_http_client->setTimeout(2*MILLISECOND_DURATION_1000);
      #endif

      __http_service.m_http_client->addHeader("Content-Type", "application/json");

      LogFmtI("posting data : %s\n", _payload->c_str());

      int _httpCode = __http_service.m_http_client->POST( *_payload );
      status = __http_service.followHttpRequest( _httpCode );

      delete _payload;

      if( status ){

        status = this->handleGpioHttpRequest();
      }
    }
  }else{
    LogI("GPIO Http Request not initializing or failed or Not Configured Correctly\n");
  }

  return status;
}

/**
 * append gpio payload to string arg
 *
 * @param String& _payload
 */
void GpioServiceProvider::appendGpioJsonPayload( String& _payload, bool isAlertPost ){

  _payload += "{";

  if( nullptr != this->m_wifi ){

    _payload += "\"";
    _payload += GPIO_PAYLOAD_MAC_KEY;
    _payload += "\":\"";
    _payload += this->m_wifi->macAddress();
    _payload += "\",";
  }

  if( isAlertPost ){

    _payload += "\"";
    _payload += GPIO_ALERT_PIN_KEY;
    _payload += "\":\"";

    if( __gpio_alert_track.alert_gpio_pin < MAX_DIGITAL_GPIO_PINS ){
      _payload += "D";
      _payload += __gpio_alert_track.alert_gpio_pin;
    }else{
      _payload += "A";
      _payload += (__gpio_alert_track.alert_gpio_pin - MAX_DIGITAL_GPIO_PINS);
    }

    _payload += "\",";
  }

  _payload += "\"";
  _payload += GPIO_PAYLOAD_DATA_KEY;
  _payload += "\":{";

  for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS; _pin++) {

    if( !this->is_exceptional_gpio_pin(_pin) ){

      _payload += "\"D";
      _payload += _pin;
      _payload += "\":{\"";
      _payload += GPIO_PAYLOAD_MODE_KEY;
      _payload += "\":";
      _payload += this->m_gpio_config_copy.gpio_mode[_pin];
      _payload += ",\"";
      _payload += GPIO_PAYLOAD_VALUE_KEY;
      _payload += "\":";
      _payload += this->m_gpio_config_copy.gpio_readings[_pin];
      _payload += "},";
    }
  }

  for (uint8_t _pin = 0; _pin < MAX_ANALOG_GPIO_PINS; _pin++) {

    _payload += "\"A";
    _payload += _pin;
    _payload += "\":{\"";
    _payload += GPIO_PAYLOAD_MODE_KEY;
    _payload += "\":";
    _payload += this->m_gpio_config_copy.gpio_mode[MAX_DIGITAL_GPIO_PINS+_pin];
    _payload += ",\"";
    _payload += GPIO_PAYLOAD_VALUE_KEY;
    _payload += "\":";
    _payload += this->m_gpio_config_copy.gpio_readings[MAX_DIGITAL_GPIO_PINS+_pin];
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
    for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS; _pin++) {

      _pin_label[1] = ( 0x30 + _pin ); memset( _pin_data, 0, _pin_data_max_len);
      memset( _pin_mode, 0, _pin_values_max_len); memset( _pin_value, 0, _pin_values_max_len);
      if( !this->is_exceptional_gpio_pin(_pin) && __get_from_json( _payload, _pin_label, _pin_data, _pin_data_max_len ) ){

        if( __get_from_json( _pin_data, (char*)GPIO_PAYLOAD_MODE_KEY, _pin_mode, _pin_values_max_len ) ){

          if( __get_from_json( _pin_data, (char*)GPIO_PAYLOAD_VALUE_KEY, _pin_value, _pin_values_max_len ) ){

            uint8_t _mode = StringToUint8( _pin_mode, _pin_values_max_len );
            uint16_t _value = StringToUint16( _pin_value, _pin_values_max_len );
            uint16_t _value_limit = _mode == ANALOG_WRITE ? ANALOG_GPIO_RESOLUTION : _mode == DIGITAL_BLINK ? _value+1 : GPIO_STATE_MAX;
            this->m_gpio_config_copy.gpio_mode[_pin] = _mode < ANALOG_READ ? _mode : this->m_gpio_config_copy.gpio_mode[_pin];
            this->m_gpio_config_copy.gpio_readings[_pin] = _value < _value_limit ? _value : this->m_gpio_config_copy.gpio_readings[_pin];
            __database_service.set_gpio_config_table(&this->m_gpio_config_copy);
            this->handleGpioModes();
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

  String *_payload = new String("");

  if( nullptr != _payload ){

    this->appendGpioJsonPayload( *_payload, true );

    *_payload += "\n\nRegards\n";

    if( nullptr != this->m_wifi ){

      *_payload += this->m_wifi->macAddress();
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
        delete this->m_digital_blinker[_pin];
        this->m_digital_blinker[_pin] = nullptr;
      }
    }

    switch ( this->is_exceptional_gpio_pin(_pin) ? OFF : this->m_gpio_config_copy.gpio_mode[_pin] ) {

      default:
      case OFF:{
        this->m_gpio_config_copy.gpio_readings[_pin] = LOW;
        break;
      }
      case DIGITAL_WRITE:{
        __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, this->getGpioFromPinMap( _pin ), this->m_gpio_config_copy.gpio_readings[_pin] );
        break;
      }
      case DIGITAL_READ:{
        this->m_gpio_config_copy.gpio_readings[_pin] = __i_dvc_ctrl.gpioRead(DIGITAL_READ, this->getGpioFromPinMap( _pin ) );
        break;
      }
      case DIGITAL_BLINK:{
        if( nullptr != this->m_digital_blinker[_pin] ){

          this->m_digital_blinker[_pin]->updateConfig( this->getGpioFromPinMap( _pin ), this->m_gpio_config_copy.gpio_readings[_pin] );
          this->m_digital_blinker[_pin]->start();
        }else{

          this->m_digital_blinker[_pin] = new DigitalBlinker( this->getGpioFromPinMap( _pin ), this->m_gpio_config_copy.gpio_readings[_pin] );
        }
        break;
      }
      case ANALOG_WRITE:{
        __i_dvc_ctrl.gpioWrite(ANALOG_WRITE, this->getGpioFromPinMap( _pin ), this->m_gpio_config_copy.gpio_readings[_pin] );
        break;
      }
      case ANALOG_READ:{
        if( MAX_DIGITAL_GPIO_PINS <= _pin  ){
          this->m_gpio_config_copy.gpio_readings[_pin] = __i_dvc_ctrl.gpioRead(ANALOG_READ, A0 );
        }
        break;
      }
    }

    if( !this->is_exceptional_gpio_pin(_pin) && this->m_gpio_config_copy.gpio_alert_channel[_pin] != NO_ALERT ){

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

    switch ( _gpio_configs.gpio_mode[_pin] ) {

      case OFF:
      case DIGITAL_READ:
      default:
        pinMode( this->getGpioFromPinMap( _pin ), INPUT );
        break;
      case DIGITAL_BLINK:
      case DIGITAL_WRITE:
      case ANALOG_WRITE:
        pinMode( this->getGpioFromPinMap( _pin ), OUTPUT );
        break;
      case ANALOG_READ:
      break;
    }

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

/**
 * get gpio mapped pin from its no.
 */
uint8_t GpioServiceProvider::getGpioFromPinMap( uint8_t _pin ){

  uint8_t mapped_pin;

  // Map
  switch ( _pin ) {

    case 0:
      mapped_pin = 16;
      break;
    case 1:
      mapped_pin = 5;
      break;
    case 2:
      mapped_pin = 4;
      break;
    case 3:
      mapped_pin = 0;
      break;
    case 4:
      mapped_pin = 2;
      break;
    case 5:
      mapped_pin = 14;
      break;
    case 6:
      mapped_pin = 12;
      break;
    case 7:
      mapped_pin = 13;
      break;
    case 8:
      mapped_pin = 15;
      break;
    case 9:
      mapped_pin = 3;
      break;
    case 10:
      mapped_pin = 1;
      break;
    default:
      mapped_pin = 0;
  }

  return mapped_pin;
}

bool GpioServiceProvider::is_exceptional_gpio_pin (uint8_t _pin) {

  for (uint8_t j = 0; j < sizeof(EXCEPTIONAL_GPIO_PINS); j++) {

    if( EXCEPTIONAL_GPIO_PINS[j] == _pin )return true;
  }
  return false;
}

// int getPinMode(uint8_t pin){
//
//   if (pin >= NUM_DIGITAL_PINS) return (-1);
//
//   uint8_t bit = digitalPinToBitMask(pin);
//   uint8_t port = digitalPinToPort(pin);
//   volatile uint8_t *reg = portModeRegister(port);
//   if (*reg & bit) return (OUTPUT);
//
//   volatile uint8_t *out = portOutputRegister(port);
//   return ((*out & bit) ? INPUT_PULLUP : INPUT);
// }


GpioServiceProvider __gpio_service;

#endif
