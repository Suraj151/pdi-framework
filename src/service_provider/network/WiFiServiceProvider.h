/***************************** WiFi service ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WIFI_SERVICE_PROVIDER_H_
#define _WIFI_SERVICE_PROVIDER_H_

#include <service_provider/ServiceProvider.h>
#include <service_provider/database/DatabaseServiceProvider.h>

/**
 * WiFiServiceProvider class
 */
class WiFiServiceProvider : public ServiceProvider
{

public:
  /**
   * WiFiServiceProvider constructor.
   */
  WiFiServiceProvider();

  /**
   * WiFiServiceProvider destructor
   */
  ~WiFiServiceProvider();

  bool initService(void *arg = nullptr) override;
  bool configure_wifi_access_point(wifi_config_table *_wifi_credentials);
  bool configure_wifi_station(wifi_config_table *_wifi_credentials, uint8_t *mac = nullptr);

#ifndef IGNORE_FREE_RELAY_CONNECTIONS
  void scan_aps_and_configure_wifi_station_async(int _scanCount);
#endif

#ifdef ENABLE_DYNAMIC_SUBNETTING
  void reconfigure_wifi_access_point(void);
#endif

  uint32_t getStationSubnetIP(void);
  uint32_t getStationBroadcastIP(void);

  void handleInternetConnectivity(void);
  void handleWiFiConnectivity(void);

  void printConfigToTerminal(iTerminalInterface *terminal) override;
  void printStatusToTerminal(iTerminalInterface *terminal) override;


  /**
   * @var	uint8_t  m_wifi_connection_timeout
   */
  uint8_t m_wifi_connection_timeout;
  /**
   * @var	uint8_t array temperory mac buffer
   */
  uint8_t m_temp_mac[6];

protected:
  /**
   * @var	iWiFiInterface*|&WiFi wifi
   */
  iWiFiInterface *m_wifi;
};

extern WiFiServiceProvider __wifi_service;

#endif
