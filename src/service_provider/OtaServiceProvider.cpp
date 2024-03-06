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
OtaServiceProvider::OtaServiceProvider() : m_http_client(Http_Client::GetStaticInstance())
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
void OtaServiceProvider::begin_ota(iClientInterface *_client)
{
  if (nullptr == _client)
  {
    return;
  }

  if (nullptr != this->m_http_client)
  {
    this->m_http_client->SetClient(_client);
  }

  __task_scheduler.setInterval([&]()
                               { this->handleOta(); },
                               OTA_API_CHECK_DURATION, __i_dvc_ctrl.millis_now());
}

/**
 * check for ota updates and restart device on successful updates
 *
 */
void OtaServiceProvider::handleOta()
{

  LogI("\nHandeling OTA\n");

  http_ota_status _stat = this->handle();

  LogFmtI("OTA status : %d\n", (int)_stat);

  if (_stat == UPDATE_OK)
  {
    LogI("\nOTA Done ....Rebooting\n");
    __i_dvc_ctrl.restartDevice();
  }
}

/**
 * handle ota by cheching firmware version first and update firmware if
 * latest version is available
 *
 * @return  http_ota_status
 */
http_ota_status OtaServiceProvider::handle()
{
  http_ota_status _status = UNKNOWN;
  ota_config_table _ota_configs;
  global_config_table _global_configs;
  __database_service.get_global_config_table(&_global_configs);
  __database_service.get_ota_config_table(&_ota_configs);

  std::string firmware_url = _ota_configs.ota_host;

  if (firmware_url.size() > 0)
  {
    if (firmware_url[firmware_url.size() - 1] == '/')
    {
      firmware_url.pop_back();
    }
    firmware_url += OTA_VERSION_CHECK_URL;

    size_t mac_index = firmware_url.find("[mac]");
    if (std::string::npos != mac_index)
    {
      firmware_url.replace(mac_index, 5, __i_dvc_ctrl.getDeviceMac().c_str());
    }
  }

  if (nullptr != this->m_http_client && firmware_url.size() > 0 && _ota_configs.ota_port > 0)
  {
    this->m_http_client->Begin();
    this->m_http_client->SetUserAgent("Ewings");
    this->m_http_client->SetBasicAuthorization("ota", __i_dvc_ctrl.getDeviceMac().c_str());
    this->m_http_client->SetTimeout(2 * MILLISECOND_DURATION_1000);

    int _httpCode = this->m_http_client->Get(firmware_url.c_str());

    char *http_resp = nullptr;
    int16_t httl_resp_len = 0;
    this->m_http_client->GetResponse(http_resp, httl_resp_len);

    if (HTTP_RESP_OK == _httpCode && nullptr != http_resp)
    {
      // int _rsponse_len = (httl_resp_len + 1) > OTA_VERSION_API_RESP_LENGTH ? OTA_VERSION_API_RESP_LENGTH : (httl_resp_len + 1);
      uint32_t _firm_version = 0;
      char *_version_buf = new char[OTA_VERSION_LENGTH];

      if (nullptr != _version_buf)
      {
        memset(_version_buf, 0, OTA_VERSION_LENGTH);

        if (__get_from_json(http_resp, (char *)OTA_VERSION_KEY, _version_buf, OTA_VERSION_LENGTH))
        {
          _firm_version = StringToUint32(_version_buf);

          LogFmtI("Http OTA current version : %d\n", _global_configs.firmware_version);
          LogFmtI("Http OTA got version : %d\n", _firm_version);
        }
        else
        {
          _status = VERSION_NOT_FOUND;
        }

        delete[] _version_buf;
      }

      this->m_http_client->End(true);

      if (_firm_version > _global_configs.firmware_version)
      {
        firmware_url = _ota_configs.ota_host;

        if (firmware_url.size() > 0)
        {
          if (firmware_url[firmware_url.size() - 1] == '/')
          {
            firmware_url.pop_back();
          }
          firmware_url += OTA_BINARY_DOWNLOAD_URL;

          size_t mac_index = firmware_url.find("[mac]");
          if (std::string::npos != mac_index)
          {
            firmware_url.replace(mac_index, 5, __i_dvc_ctrl.getDeviceMac().c_str());
          }

          firmware_url += std::to_string(_firm_version);
        }

        upgrade_status_t upgrd_status = __i_dvc_ctrl.Upgrade(firmware_url.c_str(), std::to_string(_global_configs.firmware_version).c_str());

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
      this->m_http_client->End(true);
      _status = GET_VERSION_FAILED;
    }
  }
  else
  {
    LogE("Http OTA Update not initializing or failed or Not Configured Correctly\n");
  }
  return _status;
}

/**
 * print ota configs
 */
void OtaServiceProvider::printOtaConfigLogs()
{
  ota_config_table _ota_configs;
  __database_service.get_ota_config_table(&_ota_configs);

  LogFmtI("\nOTA Configs :\n%s\t%d\n", _ota_configs.ota_host, _ota_configs.ota_port);
}

OtaServiceProvider __ota_service;
