/************** Over The Air firmware update service **************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_OTA_SERVICE)

#include "OtaServiceProvider.h"

#ifdef ENABLE_DEVICE_IOT
#include <service_provider/iot/DeviceIotServiceProvider.h>
#endif

/**
 * OtaServiceProvider constructor.
 */
OtaServiceProvider::OtaServiceProvider() : m_http_client(Http_Client::GetStaticInstance()), ServiceProvider(SERVICE_OTA, RODT_ATTR("OTA"))
{
}

/**
 * OtaServiceProvider destructor.
 */
OtaServiceProvider::~OtaServiceProvider()
{
  this->m_http_client = nullptr;
}

/**
 * begin ota with client and database configs
 * schedule task for ota check once in perticuler duration
 *
 * @param iClientInterface* _client
 */
bool OtaServiceProvider::initService(void *arg)
{
  iClientInterface* _client = static_cast<iClientInterface*>(arg);
  
  if (nullptr == _client)
  {
    return false;
  }

  if (nullptr != this->m_http_client)
  {
    this->m_http_client->SetClient(_client);
  }

  this->serviceSetInterval([&]()
                           { this->handleOta(); },
                           OTA_API_CHECK_DURATION, __i_dvc_ctrl.millis_now());

  return ServiceProvider::initService(arg);
}

/**
 * check for ota updates and restart device on successful updates
 *
 */
void OtaServiceProvider::handleOta()
{

  LogI("\nHandeling OTA\n");

  this->handleOtaVersionRequest();
}

/**
 * request the latest published firmware version
 *
 */
void OtaServiceProvider::handleOtaVersionRequest()
{
  // one request at a time, the client is released once its response is handled
  if( nullptr != this->m_http_client &&
      HTTP_ASYNC_IDLE != this->m_http_client->GetAsyncState() ){
    return;
  }

  ota_config_table _ota_configs;
  __database_service.get_ota_config_table(&_ota_configs);

  pdiutil::string firmware_url = _ota_configs.ota_host;

  if (firmware_url.size() > 0)
  {
    if (firmware_url[firmware_url.size() - 1] == '/')
    {
      firmware_url.pop_back();
    }
    firmware_url += CHARPTR_WRAP(OTA_VERSION_CHECK_URL);

    pdiutil::string mac_placeholder = CHARPTR_WRAP("[mac]");
    size_t mac_index = firmware_url.find(mac_placeholder.c_str());
    if (pdiutil::string::npos != mac_index)
    {
      firmware_url.replace(mac_index, 5, __i_dvc_ctrl.getDeviceMac().c_str());
    }

#ifdef ENABLE_DEVICE_IOT
    pdiutil::string duid_placeholder = CHARPTR_WRAP("[duid]");
    pdiutil::string::size_type duid_index = firmware_url.find(duid_placeholder.c_str());
    if( pdiutil::string::npos != duid_index )
    {
      firmware_url.replace( duid_index, 6, __device_iot_service.getDeviceId() );
    }
#endif
  }

  if (nullptr != this->m_http_client && firmware_url.size() > 0 && _ota_configs.ota_port > 0)
  {
    pdiutil::string user_agent = CHARPTR_WRAP("pdistack");
    pdiutil::string auth_user = CHARPTR_WRAP("ota");
    this->m_http_client->Begin();
    this->m_http_client->SetUserAgent(user_agent.c_str());
    this->m_http_client->SetBasicAuthorization(auth_user.c_str(), __i_dvc_ctrl.getDeviceMac().c_str());
    this->m_http_client->SetTimeout(2 * MILLISECOND_DURATION_1000);
    this->m_http_client->GetAsync(firmware_url.c_str(), [](void *arg){
      __ota_service.handleOtaVersionResponse(reinterpret_cast<Http_Client*>(arg));
    });
  }
  else
  {
    SysLogE("Http OTA Update not initializing or failed or Not Configured Correctly\n");
  }
}

/**
 * handle the version response and upgrade the firmware if a newer one is published
 *
 */
void OtaServiceProvider::handleOtaVersionResponse( Http_Client *client )
{
  if( nullptr == client ){
    return;
  }

  http_ota_status _status = UNKNOWN;
  ota_config_table _ota_configs;
  global_config_table _global_configs;
  __database_service.get_global_config_table(&_global_configs);
  __database_service.get_ota_config_table(&_ota_configs);

  pdiutil::string firmware_url;
  int16_t _httpCode = client->GetRespStatusCode();
  char *http_resp = nullptr;
  int16_t httl_resp_len = 0;
  client->GetResponse(http_resp, httl_resp_len);

  if (HTTP_RESP_OK == _httpCode && nullptr != http_resp)
  {
    uint32_t _firm_version = 0;
    char *_version_buf = pdiutil::safe_new_array<char>(OTA_VERSION_LENGTH);

    if (nullptr != _version_buf)
    {
      memset(_version_buf, 0, OTA_VERSION_LENGTH);

      pdiutil::string version_key = CHARPTR_WRAP(OTA_VERSION_KEY);
      if (__get_from_json(http_resp, version_key.c_str(), _version_buf, OTA_VERSION_LENGTH))
      {
        _firm_version = StringToUint32(_version_buf);

        LogI("Http OTA current version : %d\n", _global_configs.firmware_version);
        LogI("Http OTA got version : %d\n", _firm_version);
      }
      else
      {
        _status = VERSION_NOT_FOUND;
      }

      pdiutil::safe_delete_array(_version_buf);
    }

    client->End(true);

    if (_firm_version > _global_configs.firmware_version)
    {
      firmware_url = _ota_configs.ota_host;

      if (firmware_url.size() > 0)
      {
        if (firmware_url[firmware_url.size() - 1] == '/')
        {
          firmware_url.pop_back();
        }
        firmware_url += CHARPTR_WRAP(OTA_BINARY_DOWNLOAD_URL);

        pdiutil::string mac_placeholder = CHARPTR_WRAP("[mac]");
        size_t mac_index = firmware_url.find(mac_placeholder.c_str());
        if (pdiutil::string::npos != mac_index)
        {
          firmware_url.replace(mac_index, 5, __i_dvc_ctrl.getDeviceMac().c_str());
        }

#ifdef ENABLE_DEVICE_IOT
        pdiutil::string duid_placeholder = CHARPTR_WRAP("[duid]");
        pdiutil::string::size_type duid_index = firmware_url.find(duid_placeholder.c_str());
        if (pdiutil::string::npos != duid_index)
        {
          firmware_url.replace(duid_index, 6, __device_iot_service.getDeviceId());
        }
#endif
        firmware_url += pdiutil::to_string(_firm_version);
      }

      LogI("Starting OTA...\n");

      pdiutil::string user_agent = CHARPTR_WRAP("pdistack");
      pdiutil::string auth_user = CHARPTR_WRAP("ota");
      client->Begin();
      client->SetUserAgent(user_agent.c_str());
      client->SetBasicAuthorization(auth_user.c_str(), __i_dvc_ctrl.getDeviceMac().c_str());
      client->SetTimeout(120 * MILLISECOND_DURATION_1000);
      upgrade_status_t upgrd_status = __i_dvc_ctrl.Upgrade(
          firmware_url.c_str(),
          pdiutil::to_string(_global_configs.firmware_version).c_str(),
          client
      );
      client->End(true);

      if (upgrd_status == UPGRADE_STATUS_FAILED)
      {
        _status = UPDATE_FAILD;
      }
      else if (upgrd_status == UPGRADE_STATUS_IGNORE)
      {
        _status = NO_UPDATES;
      }
      else if (upgrd_status == UPGRADE_STATUS_SUCCESS)
      {
        _global_configs.firmware_version = _firm_version;
        __database_service.set_global_config_table(&_global_configs);
        _status = UPDATE_OK;
      }
    }
    else
    {
      _status = NO_UPDATES;
    }
  }
  else
  {
    client->End(true);
    _status = GET_VERSION_FAILED;
  }

  LogI("OTA status : %d\n", (int)_status);

  if (_status == UPDATE_OK)
  {
    LogI("\nOTA Done ....Rebooting\n");
    __i_dvc_ctrl.restartDevice();
  }
}

void OtaServiceProvider::setHttpHost(const char* _host)
{
  int16_t len = strlen(_host);

  if (len < OTA_HOST_BUF_SIZE) {

    ota_config_table _ota_configs;
    __database_service.get_ota_config_table(&_ota_configs);
    memset(_ota_configs.ota_host, 0, OTA_HOST_BUF_SIZE);
    memcpy(_ota_configs.ota_host, _host, len);
    __database_service.set_ota_config_table(&_ota_configs);
  }
}

/**
 * print Ota configs to terminal
 */
void OtaServiceProvider::printConfigToTerminal(iTerminalInterface *terminal)
{
  if( nullptr != terminal ){

    ota_config_table _ota_configs;
    __database_service.get_ota_config_table(&_ota_configs);

    terminal->writeln();
    terminal->writeln_ro(RODT_ATTR("OTA Configs :"));
    terminal->write(_ota_configs.ota_host);
    terminal->write_ro(RODT_ATTR("\t"));
    terminal->writeln((int32_t)_ota_configs.ota_port);
  }
}

OtaServiceProvider __ota_service;

#endif