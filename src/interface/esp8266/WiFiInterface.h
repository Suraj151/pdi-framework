/**************************** WiFi Interface ***********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WIFI_INTERFACE_H_
#define _WIFI_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/modules/wifi/iWiFiInterface.h>
#include "LoggerInterface.h"
#include "DeviceControlInterface.h"

// declare device specific
class IPAddress;

/**
 * WiFiInterface class
 */
class WiFiInterface : public iWiFiInterface
{

public:
  /**
   * WiFiInterface constructor.
   */
  WiFiInterface();
  /**
   * WiFiInterface destructor.
   */
  ~WiFiInterface();

  // generic api's
  void setOutputPower(float _dBm) override;
  void persistent(bool _persistent) override;
  bool setMode(wifi_mode_t _mode) override;
  wifi_mode_t getMode() override;
  bool setSleepMode(sleep_mode_t type, uint8_t listenInterval = 0) override;
  sleep_mode_t getSleepMode() override;
  bool enableSTA(bool _enable) override;
  bool enableAP(bool _enable) override;
  int hostByName(const char *aHostname, ipaddress_t &aResult, uint32_t timeout_ms) override;

  wifi_status_t begin(char *_ssid, char *_passphrase = nullptr, int32_t _channel = 0, const uint8_t *_bssid = nullptr, bool _connect = true) override;
  bool config(ipaddress_t &_local_ip, ipaddress_t &_gateway, ipaddress_t &_subnet) override;
  bool reconnect() override;
  bool disconnect(bool _wifioff = false) override;
  bool isConnected() override;
  bool setAutoReconnect(bool _autoReconnect) override;

  // STA network info
  ipaddress_t localIP() override;
  std::string macAddress() override;
  ipaddress_t subnetMask() override;
  ipaddress_t gatewayIP() override;
  ipaddress_t dnsIP(uint8_t _dns_no = 0) override;

  // STA WiFi info
  wifi_status_t status() override;
  std::string SSID() const override;
  uint8_t *BSSID() override;
  int32_t RSSI() override;

  // Soft AP api's
  bool softAP(const char *_ssid, const char *_passphrase = nullptr, int _channel = 1, int _ssid_hidden = 0, int _max_connection = 4) override;
  bool softAPConfig(ipaddress_t _local_ip, ipaddress_t _gateway, ipaddress_t _subnet) override;
  bool softAPdisconnect(bool _wifioff = false) override;
  ipaddress_t softAPIP() override;

  // n/w scan api's
  int8_t scanNetworks(bool _async = false, bool _show_hidden = false, uint8_t _channel = 0, uint8_t *ssid = nullptr) override;
  void scanNetworksAsync(std::function<void(int)> _onComplete, bool _show_hidden = false) override;
  std::string SSID(uint8_t _networkItem) override;
  int32_t RSSI(uint8_t _networkItem) override;
  uint8_t *BSSID(uint8_t _networkItem) override;
  bool get_bssid_within_scanned_nw_ignoring_connected_stations(char *ssid, uint8_t *bssid, uint8_t *ignorebssid, int _scanCount) override;
  bool getApsConnectedStations(std::vector<wifi_station_info_t> &stations) override;

  void enableNetworkStatusIndication() override;
  void enableNAPT() override;
  static void wifi_event_handler_cb( System_Event_t * _event );

  // misc api's
  static void preinitWiFiOff();

private:
  /**
   * @var	ESP8266WiFiClass*|&WiFi m_wifi
   */
  ESP8266WiFiClass *m_wifi;

  /**
   * @var	int16_t for network indication
   */
  int16_t   m_wifi_led;
};

#endif
