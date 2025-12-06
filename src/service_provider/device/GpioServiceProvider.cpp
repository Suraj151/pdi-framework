/****************************** Gpio service **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include <config/Config.h>

#if defined(ENABLE_GPIO_SERVICE)


#include "GpioServiceProvider.h"


#ifndef ENABLE_GPIO_BASIC_ONLY
__gpio_alert_track_t __gpio_alert_track = {
  false, 0, -1
};
#endif

/**
 * GpioServiceProvider constructor.
 */
GpioServiceProvider::GpioServiceProvider():
  m_gpio_http_request_cb_id(0),
  m_update_gpio_table_from_copy(true),
  #ifdef ENABLE_HTTP_CLIENT
  m_http_client(Http_Client::GetStaticInstance()),
  #endif
  ServiceProvider(SERVICE_GPIO, RODT_ATTR("GPIO"))
{
  for (size_t i = 0; i < MAX_DIGITAL_GPIO_PINS; i++) {
    this->m_digital_blinker[i] = nullptr;
  }
}

/**
 * GpioServiceProvider destructor
 */
GpioServiceProvider::~GpioServiceProvider(){

#ifdef ENABLE_HTTP_CLIENT
  this->m_http_client = nullptr;
#endif

  for (size_t i = 0; i < MAX_DIGITAL_GPIO_PINS; i++) {
    if( nullptr != this->m_digital_blinker[i] ){
      __i_dvc_ctrl.releaseGpioBlinkerInstance(this->m_digital_blinker[i]);
      this->m_digital_blinker[i] = nullptr;
    }
  }
}

/**
 * Init gpio services if enabled
 */
bool GpioServiceProvider::initService(void *arg){

#ifdef ENABLE_HTTP_CLIENT
  iClientInterface* _client = static_cast<iClientInterface*>(arg);

  if( nullptr == _client ){
    return false;
  }

  if( nullptr != this->m_http_client ){
    this->m_http_client->SetClient(_client);
  }
#endif

  this->handleGpioModes();
  __database_service.get_gpio_config_table(&this->m_gpio_config_copy);

  __task_scheduler.setInterval( [&]() { this->handleGpioOperations(); }, GPIO_OPERATION_DURATION, __i_dvc_ctrl.millis_now() );
  __task_scheduler.setInterval( [&]() { this->enable_update_gpio_table_from_copy(); }, GPIO_TABLE_UPDATE_DURATION, __i_dvc_ctrl.millis_now() );

  return ServiceProvider::initService(arg);
}

#ifdef ENABLE_HTTP_CLIENT
/**
 * post gpio data to server specified in gpio configs
 */
bool GpioServiceProvider::handleGpioHttpRequest( bool isAlertPost ){

  bool status = false;

  pdiutil::string posturl = this->m_gpio_config_copy.gpio_host;

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

    pdiutil::string::size_type mac_index = posturl.find("[mac]");
    if( pdiutil::string::npos != mac_index )
    {
      posturl.replace( mac_index, 5, __i_dvc_ctrl.getDeviceMac().c_str() );
    }

    pdiutil::string::size_type duid_index = posturl.find("[duid]");
    if( pdiutil::string::npos != duid_index )
    {
      posturl.replace( duid_index, 6, m_device_id.c_str() );
    }

    pdiutil::string *_payload = new pdiutil::string();

    if( nullptr != _payload ){

      this->appendGpioJsonPayload( *_payload, isAlertPost );

      this->m_http_client->Begin();

      #ifdef ENABLE_DEVICE_IOT
      this->m_http_client->SetUserAgent("pdistack");
      this->m_http_client->SetBasicAuthorization("mac", __i_dvc_ctrl.getDeviceMac().c_str());
      #else
      this->m_http_client->SetUserAgent("pdistack");
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
#endif

/**
 * append gpio payload to string arg
 *
 * @param pdiutil::string& _payload
 */
void GpioServiceProvider::appendGpioJsonPayload( pdiutil::string &_payload, bool isAlertPost, pdiutil::vector<pdiutil::string> *allowedlist ){

  _payload += "{";

  if( __i_dvc_ctrl.getDeviceMac().size() ){

    _payload += "\"";
    _payload += GPIO_PAYLOAD_MAC_KEY;
    _payload += "\":\"";
    _payload += __i_dvc_ctrl.getDeviceMac().c_str();
    _payload += "\",";
  }

#ifdef ENABLE_DEVICE_IOT

  if( m_device_id.size() > 0 ){

    _payload += "\"";
    _payload += GPIO_PAYLOAD_DUID_KEY;
    _payload += "\":\"";
    _payload += m_device_id.c_str();
    _payload += "\",";
  }
#endif

#ifndef ENABLE_GPIO_BASIC_ONLY
  if( isAlertPost ){

    _payload += "\"";
    _payload += GPIO_ALERT_PIN_KEY;
    _payload += "\":\"";

    if( __gpio_alert_track.alert_gpio_pin < MAX_DIGITAL_GPIO_PINS ){
      _payload += "D";
      _payload += pdiutil::to_string(__gpio_alert_track.alert_gpio_pin);
    }else{
      _payload += "A";
      _payload += pdiutil::to_string(__gpio_alert_track.alert_gpio_pin - MAX_DIGITAL_GPIO_PINS);
    }

    _payload += "\",";
  }
#endif

  _payload += "\"";
  _payload += GPIO_PAYLOAD_DATA_KEY;
  _payload += "\":{";

  bool _remove_comma = false;
  for (uint8_t _pin = 0; _pin < MAX_DIGITAL_GPIO_PINS; _pin++) {

    if( !__i_dvc_ctrl.isExceptionalGpio(_pin) && this->isAllowedGpioPin(_pin, allowedlist) ){

      _payload += "\"D";
      _payload += pdiutil::to_string(_pin);
      _payload += "\":{\"";
      _payload += GPIO_PAYLOAD_MODE_KEY;
      _payload += "\":";
      _payload += pdiutil::to_string(this->m_gpio_config_copy.gpio_mode[_pin]);
      _payload += ",\"";
      _payload += GPIO_PAYLOAD_VALUE_KEY;
      _payload += "\":";
      _payload += pdiutil::to_string(this->m_gpio_config_copy.gpio_readings[_pin]);
      _payload += "},";

      _remove_comma = true;
    }
  }

  for (uint8_t _pin = 0; _pin < MAX_ANALOG_GPIO_PINS; _pin++) {

    if( this->isAllowedGpioPin(MAX_DIGITAL_GPIO_PINS+_pin, allowedlist) ){

      _payload += "\"A";
      _payload += pdiutil::to_string(_pin);
      _payload += "\":{\"";
      _payload += GPIO_PAYLOAD_MODE_KEY;
      _payload += "\":";
      _payload += pdiutil::to_string(this->m_gpio_config_copy.gpio_mode[MAX_DIGITAL_GPIO_PINS+_pin]);
      _payload += ",\"";
      _payload += GPIO_PAYLOAD_VALUE_KEY;
      _payload += "\":";
      _payload += pdiutil::to_string(this->m_gpio_config_copy.gpio_readings[MAX_DIGITAL_GPIO_PINS+_pin]);
      _payload += "},";

      _remove_comma = true;
    }
  }

  if( _remove_comma ){
    _payload.pop_back(); // remove last comma
  }

  _payload += "}}";
}

/**
 * apply json payload to gpio operations
 *
 * @param char* _payload
 */
void GpioServiceProvider::applyGpioJsonPayload( char* _payload, uint16_t _payload_length, pdiutil::vector<pdiutil::string> *allowedlist ){

  LogFmtI("Applying GPIO from Json Payload : %s\n", _payload);

  if(
    0 <= __strstr( _payload, (char*)GPIO_PAYLOAD_DATA_KEY, _payload_length - strlen(GPIO_PAYLOAD_DATA_KEY) ) &&
    0 <= __strstr( _payload, (char*)GPIO_PAYLOAD_MODE_KEY, _payload_length - strlen(GPIO_PAYLOAD_MODE_KEY) ) &&
    0 <= __strstr( _payload, (char*)GPIO_PAYLOAD_VALUE_KEY, _payload_length - strlen(GPIO_PAYLOAD_VALUE_KEY) )
  ){

    int _pin_data_max_len = 30, _pin_values_max_len = 6;
    char _pin_label_uppercase[_pin_values_max_len]; //memset( _pin_label, 0, _pin_values_max_len); _pin_label[0] = 'D';
    char _pin_label_lowercase[_pin_values_max_len];
    char _pin_data[_pin_data_max_len], _pin_mode[_pin_values_max_len], _pin_value[_pin_values_max_len];
    // Decode the pin mode and value
    for (uint8_t _pin = 0; _pin < (MAX_GPIO_PINS); _pin++) {

      uint8_t _pin_label_n = _pin;

      memset( _pin_label_uppercase, 0, _pin_values_max_len);
      memset( _pin_label_lowercase, 0, _pin_values_max_len);
      if( _pin < MAX_DIGITAL_GPIO_PINS ){

        __appendUintToBuff(_pin_label_uppercase, "D%d", _pin_label_n, _pin_values_max_len-1);
        __appendUintToBuff(_pin_label_lowercase, "d%d", _pin_label_n, _pin_values_max_len-1);
      }else{
        _pin_label_n = _pin - MAX_DIGITAL_GPIO_PINS;

        __appendUintToBuff(_pin_label_uppercase, "A%d", _pin_label_n, _pin_values_max_len-1);
        __appendUintToBuff(_pin_label_lowercase, "a%d", _pin_label_n, _pin_values_max_len-1);
      }

      memset( _pin_data, 0, _pin_data_max_len);
      memset( _pin_mode, 0, _pin_values_max_len); 
      memset( _pin_value, 0, _pin_values_max_len);
      if( !__i_dvc_ctrl.isExceptionalGpio(_pin) && (__get_from_json( _payload, _pin_label_uppercase, _pin_data, _pin_data_max_len ) || __get_from_json( _payload, _pin_label_lowercase, _pin_data, _pin_data_max_len )) ){

        if( allowedlist != nullptr ){

          bool _is_allowed = false;

          for( size_t i=0; i < allowedlist->size(); i++ ){

            if( __are_str_equals( allowedlist->at(i).c_str(), _pin_label_uppercase, strlen( _pin_label_uppercase ) ) ||
                __are_str_equals( allowedlist->at(i).c_str(), _pin_label_lowercase, strlen( _pin_label_lowercase ) ) ){

              _is_allowed = true;
              break;
            }
          }

          if( !_is_allowed ){

            continue;
          }
        }

        if( __get_from_json( _pin_data, (char*)GPIO_PAYLOAD_MODE_KEY, _pin_mode, _pin_values_max_len ) ){

          if( __get_from_json( _pin_data, (char*)GPIO_PAYLOAD_VALUE_KEY, _pin_value, _pin_values_max_len ) ){

            LogFmtI("Applying to : %s, mode : %s, value : %s\n", _pin_label_uppercase, _pin_mode, _pin_value);

            if( !__are_arrays_equal(_pin_mode, NOT_APPLICABLE, 2) ){

              uint8_t _mode = StringToUint8( _pin_mode, _pin_values_max_len );
              this->m_gpio_config_copy.gpio_mode[_pin] = _mode < GPIO_MODE_MAX ? _mode : this->m_gpio_config_copy.gpio_mode[_pin];
              this->m_update_gpio_table_from_copy = true;
            }

            if( !__are_arrays_equal(_pin_value, NOT_APPLICABLE, 2) ){

              uint16_t _value = StringToUint16( _pin_value, _pin_values_max_len );
              uint16_t _value_limit = this->m_gpio_config_copy.gpio_mode[_pin] == ANALOG_WRITE ? ANALOG_GPIO_RESOLUTION : this->m_gpio_config_copy.gpio_mode[_pin] == DIGITAL_BLINK ? _value+1 : GPIO_STATE_MAX;
              this->m_gpio_config_copy.gpio_readings[_pin] = _value < _value_limit ? _value : this->m_gpio_config_copy.gpio_readings[_pin];
              this->m_update_gpio_table_from_copy = true;
            }
          }
        }
      }
    }

    if( this->m_update_gpio_table_from_copy ){

      for (uint8_t _pin = 0; _pin < MAX_GPIO_PINS; _pin++) {

        if( !__i_dvc_ctrl.isExceptionalGpio(_pin) ){
          __i_dvc_ctrl.gpioMode((GPIO_MODE)this->m_gpio_config_copy.gpio_mode[_pin], _pin);
        }
      }
    }
  }

}
#ifndef ENABLE_GPIO_BASIC_ONLY
/**
 * apply alert json payload to gpio operations
 *
 * @param char* _payload
 */
void GpioServiceProvider::applyGpioAlertJsonPayload( char* _payload, uint16_t _payload_length, pdiutil::vector<pdiutil::string> *allowedlist ){

  LogFmtI("Applying GPIO Alert from Json Payload : %s\n", _payload);

  if(
    0 <= __strstr( _payload, (char*)GPIO_ALERT_COMPARATOR_KEY, _payload_length - strlen(GPIO_PAYLOAD_DATA_KEY) ) &&
    0 <= __strstr( _payload, (char*)GPIO_PAYLOAD_VALUE_KEY, _payload_length - strlen(GPIO_PAYLOAD_VALUE_KEY) )
  ){

    int _iface_data_max_len = 30, _iface_keys_max_len = 6;
    char _iface_label_uppercase[_iface_keys_max_len]; //memset( _iface_label, 0, _iface_keys_max_len); _iface_label[0] = 'D';
    char _iface_label_lowercase[_iface_keys_max_len];
    char _iface_data[_iface_data_max_len], _iface_comparator[_iface_keys_max_len], _iface_value[_iface_keys_max_len];

    for (uint8_t _pin = 0; _pin < (MAX_DIGITAL_GPIO_PINS + MAX_ANALOG_GPIO_PINS); _pin++) {

      if( !__i_dvc_ctrl.isExceptionalGpio(_pin) && this->isAllowedGpioPin(_pin, allowedlist) ){

        memset( _iface_data, 0, _iface_data_max_len);
        memset( _iface_comparator, 0, _iface_keys_max_len); 
        memset( _iface_value, 0, _iface_keys_max_len);

        if( _pin < MAX_DIGITAL_GPIO_PINS ){
          __get_iface_key_informat("D", _pin, _iface_label_uppercase, _iface_label_lowercase, _iface_keys_max_len);
        }else{
          __get_iface_key_informat("A", _pin - MAX_DIGITAL_GPIO_PINS, _iface_label_uppercase, _iface_label_lowercase, _iface_keys_max_len);
        }        

        if( 
          __get_from_json( _payload, _iface_label_uppercase, _iface_data, _iface_data_max_len ) || 
          __get_from_json( _payload, _iface_label_lowercase, _iface_data, _iface_data_max_len ) 
        ){

          if( __get_from_json( _iface_data, (char*)GPIO_ALERT_COMPARATOR_KEY, _iface_comparator, _iface_keys_max_len ) ){

            if( __get_from_json( _iface_data, (char*)GPIO_PAYLOAD_VALUE_KEY, _iface_value, _iface_keys_max_len ) ){

              LogFmtI("Applying to : %s, cmp : %s, value : %s\n", _iface_label_uppercase, _iface_comparator, _iface_value);

              uint8_t _comparator = StringToUint8( _iface_comparator, _iface_keys_max_len );
              uint16_t _value = StringToUint16( _iface_value, _iface_keys_max_len );

              this->m_gpio_config_copy.updateGpioEvent(_pin, HTTP_SERVER, _comparator, _value);
              this->m_update_gpio_table_from_copy = true;
            }
          }
        }else{

          this->m_gpio_config_copy.clearGpioEvents(_pin);
        }
      }else{

        this->m_gpio_config_copy.clearGpioEvents(_pin);
      }
    }
  }
}
#endif

/**
 * Set the device id
 *
 * @param const char* _id
 */
void GpioServiceProvider::setDeviceId(const char* _id){

  m_device_id = _id;
}

/**
 * Set the Http host for gpio data/events
 *
 * @param const char* _host
 */
void GpioServiceProvider::setHttpHost(const char* _host){

  if( strlen(_host) < GPIO_HOST_BUF_SIZE ){

    memcpy( this->m_gpio_config_copy.gpio_host, _host, strlen(_host) );
    this->m_update_gpio_table_from_copy = true;
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

  pdiutil::string *_payload = new pdiutil::string();

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

  for (uint8_t _pin = 0; _pin < MAX_GPIO_PINS; _pin++) {

    if( DIGITAL_BLINK != this->m_gpio_config_copy.gpio_mode[_pin] ){

      if( _pin < MAX_DIGITAL_GPIO_PINS && nullptr != this->m_digital_blinker[_pin] ){
        __i_dvc_ctrl.releaseGpioBlinkerInstance(this->m_digital_blinker[_pin]);
        this->m_digital_blinker[_pin] = nullptr;
      }
    }

    switch ( __i_dvc_ctrl.isExceptionalGpio(_pin) ? OFF : this->m_gpio_config_copy.gpio_mode[_pin] ) {

      default:
      case OFF:{
        this->m_gpio_config_copy.gpio_readings[_pin] = 0;
        #ifndef ENABLE_GPIO_BASIC_ONLY
        this->m_gpio_config_copy.clearGpioEvents(_pin);
        #endif
        break;
      }
      case DIGITAL_WRITE:{
        __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, _pin, this->m_gpio_config_copy.gpio_readings[_pin] );
        break;
      }
      case DIGITAL_READ:{
        this->m_gpio_config_copy.gpio_readings[_pin] = __i_dvc_ctrl.gpioRead(DIGITAL_READ, _pin);
        break;
      }
      case DIGITAL_BLINK:{
        if(_pin < MAX_DIGITAL_GPIO_PINS){
          if( nullptr != this->m_digital_blinker[_pin] ){

            this->m_digital_blinker[_pin]->updateConfig( _pin, this->m_gpio_config_copy.gpio_readings[_pin] );
            this->m_digital_blinker[_pin]->start();
          }else{

            this->m_digital_blinker[_pin] = __i_dvc_ctrl.createGpioBlinkerInstance( _pin, this->m_gpio_config_copy.gpio_readings[_pin] );
          }
        }else{
          LogFmtW("\nOut Of Range GPIO blink Config : %d", _pin);
        }
        break;
      }
      case ANALOG_WRITE:{
        __i_dvc_ctrl.gpioWrite(ANALOG_WRITE, _pin, this->m_gpio_config_copy.gpio_readings[_pin] );
        break;
      }
      case ANALOG_READ:{
        if( MAX_DIGITAL_GPIO_PINS <= _pin  ){
          this->m_gpio_config_copy.gpio_readings[_pin] = __i_dvc_ctrl.gpioRead(ANALOG_READ,(_pin-MAX_DIGITAL_GPIO_PINS));
        }
        break;
      }
    }
  }

#ifndef ENABLE_GPIO_BASIC_ONLY
  for (uint8_t _evtidx = 0; _evtidx < MAX_GPIO_EVENTS; _evtidx++) {

    uint8_t _gpionumber = this->m_gpio_config_copy.gpio_events[_evtidx].gpioNumber;

    if( !__i_dvc_ctrl.isExceptionalGpio(_gpionumber) && 
      this->m_gpio_config_copy.gpio_events[_evtidx].eventChannel != GPIO_EVENT_CHANNEL_MAX 
    ){

      bool _is_alert_condition = this->m_gpio_config_copy.gpio_events[_evtidx].isEventOccur(
        this->m_gpio_config_copy.gpio_readings[_gpionumber]
      );

      uint32_t _now = __i_dvc_ctrl.millis_now();
      if( _is_alert_condition && (( __gpio_alert_track.is_last_alert_succeed ?
        GPIO_ALERT_DURATION_FOR_SUCCEED < ( _now - __gpio_alert_track.last_alert_millis ) :
        GPIO_ALERT_DURATION_FOR_FAILED < ( _now - __gpio_alert_track.last_alert_millis )
      ) || __gpio_alert_track.alert_gpio_pin == -1) ){

        __gpio_alert_track.alert_gpio_pin = _gpionumber;
        __gpio_alert_track.last_alert_millis = _now;
        switch ( this->m_gpio_config_copy.gpio_events[_evtidx].eventChannel ) {

          #ifdef ENABLE_EMAIL_SERVICE
          case EMAIL:{
            __gpio_alert_track.is_last_alert_succeed = this->handleGpioEmailAlert();
            break;
          }
          #endif
          #ifdef ENABLE_HTTP_CLIENT
          case HTTP_SERVER:{
            __gpio_alert_track.is_last_alert_succeed = this->handleGpioHttpRequest(true);
            break;
          }
          #endif
          default: break;
        }
      }
    }
  }
#endif

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

  for (uint8_t _pin = 0; _pin < MAX_GPIO_PINS; _pin++) {

    if( !__i_dvc_ctrl.isExceptionalGpio(_pin) ){
      __i_dvc_ctrl.gpioMode((GPIO_MODE)_gpio_configs.gpio_mode[_pin], _pin);
    }
  }

  this->m_gpio_config_copy = _gpio_configs;

#ifdef ENABLE_HTTP_CLIENT
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
#endif

}

/**
 * check if gpio pin is allowed in allowedlist
 *
 * @param uint8_t _pin
 * @param pdiutil::vector<pdiutil::string>* allowedlist
 * @return bool
 */
bool GpioServiceProvider::isAllowedGpioPin(uint8_t _pin, pdiutil::vector<pdiutil::string> *allowedlist){

  int _iface_keys_max_len = 6;
  char _iface_label_uppercase[_iface_keys_max_len]; //memset( _pin_label, 0, _iface_keys_max_len); _pin_label[0] = 'D';
  char _iface_label_lowercase[_iface_keys_max_len];
  uint8_t _pin_label_n = _pin;

  if( _pin < MAX_DIGITAL_GPIO_PINS ){

    __get_iface_key_informat("D", _pin_label_n, _iface_label_uppercase, _iface_label_lowercase, _iface_keys_max_len);
  }else{
    _pin_label_n = _pin - MAX_DIGITAL_GPIO_PINS;

    __get_iface_key_informat("A", _pin_label_n, _iface_label_uppercase, _iface_label_lowercase, _iface_keys_max_len);
  }

  if( allowedlist != nullptr ){

    for( size_t i=0; i < allowedlist->size(); i++ ){

      if( __are_str_equals( allowedlist->at(i).c_str(), _iface_label_uppercase, strlen( _iface_label_uppercase ) ) ||
          __are_str_equals( allowedlist->at(i).c_str(), _iface_label_lowercase, strlen( _iface_label_lowercase ) ) ){

        return true;
      }
    }

    return false;
  }

  return true;
}

/**
 * print gpio configs to terminal
 */
void GpioServiceProvider::printConfigToTerminal(iTerminalInterface *terminal)
{
  if( nullptr != terminal ){

    terminal->writeln();
    terminal->writeln_ro(RODT_ATTR("GPIO Configs (mode) :"));
    for (uint8_t _pin = 0; _pin < MAX_GPIO_PINS; _pin++) {
      terminal->write((int32_t)this->m_gpio_config_copy.gpio_mode[_pin]);
      terminal->write_ro(RODT_ATTR("\t"));
    }

    terminal->writeln();
    terminal->writeln_ro(RODT_ATTR("GPIO Configs (readings) :"));
    for (uint8_t _pin = 0; _pin < MAX_GPIO_PINS; _pin++) {
      terminal->write((int32_t)this->m_gpio_config_copy.gpio_readings[_pin]);
      terminal->write_ro(RODT_ATTR("\t"));
    }

#ifndef ENABLE_GPIO_BASIC_ONLY

    terminal->writeln();
    terminal->writeln_ro(RODT_ATTR("GPIO Configs (events) :"));
    for (uint8_t _evtidx = 0; _evtidx < MAX_GPIO_EVENTS; _evtidx++) {

			uint8_t gpionumber = this->m_gpio_config_copy.gpio_events[_evtidx].gpioNumber;

			if( this->m_gpio_config_copy.gpioHasEvents(gpionumber) ){

        if( gpionumber < MAX_DIGITAL_GPIO_PINS ){
          
          terminal->write_ro(RODT_ATTR("D"));
          terminal->write((int32_t)gpionumber);
        }else{

          terminal->write_ro(RODT_ATTR("A"));
          terminal->write((int32_t)gpionumber-MAX_DIGITAL_GPIO_PINS);
        }
        terminal->write_ro(RODT_ATTR("\t"));
        terminal->write((int32_t)this->m_gpio_config_copy.gpio_events[_evtidx].gpioNumber);
        terminal->write_ro(RODT_ATTR("\t"));
        terminal->write((int32_t)this->m_gpio_config_copy.gpio_events[_evtidx].eventCondition);
        terminal->write_ro(RODT_ATTR("\t"));
        terminal->write((int32_t)this->m_gpio_config_copy.gpio_events[_evtidx].eventConditionValue);
        terminal->write_ro(RODT_ATTR("\t"));
        terminal->writeln((int32_t)this->m_gpio_config_copy.gpio_events[_evtidx].eventChannel);
      }
    }

    terminal->writeln_ro(RODT_ATTR("GPIO Configs (server) :"));
    terminal->write(this->m_gpio_config_copy.gpio_host);
    terminal->write_ro(RODT_ATTR("\t"));
    terminal->write((int32_t)this->m_gpio_config_copy.gpio_port);
    terminal->write_ro(RODT_ATTR("\t"));
    terminal->writeln((int32_t)this->m_gpio_config_copy.gpio_post_frequency);
#endif
  }
}

GpioServiceProvider __gpio_service;

#endif
