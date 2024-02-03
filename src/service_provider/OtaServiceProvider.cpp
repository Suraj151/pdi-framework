/************** Over The Air firmware update service **************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "OtaServiceProvider.h"

/**
 * OtaServiceProvider constructor.
 */
OtaServiceProvider::OtaServiceProvider():
  m_wifi_client(nullptr),
  m_http_client(nullptr)
{
}

/**
 * OtaServiceProvider destructor.
 */
OtaServiceProvider::~OtaServiceProvider(){
  this->m_wifi_client = nullptr;
  this->m_http_client = nullptr;
}

/**
 * begin ota with client and database configs
 * schedule task for ota check once in perticuler duration
 *
 * @param iWiFiClientInterface* _wifi_client
 * @param iHttpClientInterface*	_http_client
 * @param DefaultDatabase*      _database
 */
void OtaServiceProvider::begin_ota( iWiFiClientInterface *_wifi_client, iHttpClientInterface *_http_client ){

  this->m_wifi_client = _wifi_client;
  this->m_http_client = _http_client;

  __task_scheduler.setInterval( [&]() {  this->handleOta();  }, OTA_API_CHECK_DURATION, __i_dvc_ctrl.millis_now() );
}

/**
 * check for ota updates and restart device on successful updates
 *
 */
void OtaServiceProvider::handleOta(){

  #ifdef EW_SERIAL_LOG
  Logln( F("\nHandeling OTA") );
  #endif
  http_ota_status _stat = this->handle();
  #ifdef EW_SERIAL_LOG
  Log( F("OTA status : ") );Logln( _stat );
  #endif
  if( _stat == UPDATE_OK ){
    #ifdef EW_SERIAL_LOG
    Logln( F("\nOTA Done ....Rebooting ") );
    #endif
    __i_dvc_ctrl.restartDevice();
  }
}

/**
 * handle ota by cheching firmware version first and update firmware if
 * latest version is available
 *
 * @return  http_ota_status
 */
http_ota_status OtaServiceProvider::handle(){

   if( nullptr == this->m_wifi_client || nullptr == this->m_http_client ){
     return BEGIN_FAILED;
   }

   ota_config_table _ota_configs;
   global_config_table _global_configs;
   __database_service.get_global_config_table(&_global_configs);
   __database_service.get_ota_config_table(&_ota_configs);

   String _macstr = WiFi.macAddress();
   String _firmware_version_url = String(_ota_configs.ota_host);
   if( !_firmware_version_url.endsWith("/") ) _firmware_version_url += "/";
   _firmware_version_url += "ota?mac_id=";
   _firmware_version_url += _macstr;
   _firmware_version_url += "&version=";
   _firmware_version_url += _global_configs.firmware_version;

   if( _firmware_version_url.length() > 12 && _ota_configs.ota_port > 0 &&
     this->m_http_client->begin( *this->m_wifi_client, _firmware_version_url )
   ){

     this->m_http_client->setUserAgent("Ewings");
     this->m_http_client->setAuthorization("ota", "firmware");
     this->m_http_client->setTimeout(2000);

     int _httpCode = this->m_http_client->GET();

     #ifdef EW_SERIAL_LOG
     Log( F("Http OTA version url Response code : ") );
     Logln( _httpCode );
     #endif
     if ( HTTP_CODE_OK == _httpCode || HTTP_CODE_MOVED_PERMANENTLY == _httpCode ) {

       String _response = this->m_http_client->getString();
       this->m_http_client->end();

       int _rsponse_len = (_response.length()+1) > OTA_VERSION_API_RESP_LENGTH ? OTA_VERSION_API_RESP_LENGTH : (_response.length()+1) ;
       char *_buf = new char[_rsponse_len];
       if( nullptr == _buf ){
         return UNKNOWN;
       }
       char *_version_buf = new char[OTA_VERSION_LENGTH];
       if( nullptr == _version_buf ){
         return UNKNOWN;
       }
       memset( _buf, 0, _rsponse_len );
       memset( _version_buf, 0, OTA_VERSION_LENGTH );
       _response.toCharArray( _buf, _rsponse_len );

       if( __get_from_json( _buf, (char*)OTA_VERSION_KEY, _version_buf, OTA_VERSION_LENGTH ) ){

         uint32_t _firm_version = StringToUint32(_version_buf);
         delete[] _buf; delete[] _version_buf;

         #ifdef EW_SERIAL_LOG
         Log( F("Http OTA current version : ") );
         Logln( _global_configs.firmware_version );
         Log( F("Http OTA got version : ") );
         Logln( _firm_version );
         #endif

         if( _firm_version > _global_configs.firmware_version ){

           String _firmware_bin_url = String(_ota_configs.ota_host);
           if( !_firmware_bin_url.endsWith("/") ) _firmware_bin_url += "/";
           _firmware_bin_url += "bin/";
           _firmware_bin_url += _macstr;
           _firmware_bin_url += "/";
           _firmware_bin_url += _firm_version;
           _firmware_bin_url += ".bin";

           ESPhttpUpdate.rebootOnUpdate(false);
           ESPhttpUpdate.followRedirects(true);
           t_httpUpdate_return ret = ESPhttpUpdate.update( *(this->m_wifi_client->getWiFiClient()), _firmware_bin_url );

           if( ret == HTTP_UPDATE_FAILED ){

             #ifdef EW_SERIAL_LOG
             Logln( F("HTTP_UPDATE_FAILD") );
             #endif
             return UPDATE_FAILD;
           }else if( ret == HTTP_UPDATE_NO_UPDATES ){

             #ifdef EW_SERIAL_LOG
             Logln( F("HTTP_UPDATE_NO_UPDATES") );
             #endif
             return NO_UPDATES;
           }else if( ret == HTTP_UPDATE_OK ){

             #ifdef EW_SERIAL_LOG
             Logln( F("HTTP_UPDATE_OK") );
             #endif
             _global_configs.firmware_version = _firm_version;
             __database_service.set_global_config_table( &_global_configs );
             return UPDATE_OK;
           }else{

             #ifdef EW_SERIAL_LOG
             Logln( F("HTTP_UPDATE_UNKNOWN_RETURN") );
             #endif
             return UNKNOWN;
           }

         }else{
           return ALREADY_LATEST_VERSION;
         }
       }else{
         delete[] _buf; delete[] _version_buf;
         return VERSION_JSON_ERROR;
       }
     }else{
       this->m_http_client->end();
       return VERSION_CHECK_FAILED;
     }
   }else{
     #ifdef EW_SERIAL_LOG
     Logln( F("Http OTA Update not initializing or failed or Not Configured Correctly") );
     #endif
     return CONFIG_ERROR;
   }
}

#ifdef EW_SERIAL_LOG
/**
 * print ota configs
 */
void OtaServiceProvider::printOtaConfigLogs(){

  ota_config_table _ota_configs;
  __database_service.get_ota_config_table(&_ota_configs);

  Logln(F("\nOTA Configs :"));
  Log(_ota_configs.ota_host); Log("\t");
  Log(_ota_configs.ota_port); Logln("\n");
}
#endif

OtaServiceProvider __ota_service;
