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
#include <interface/pdi/iWiFiInterface.h>

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
  bool getPersistent() override;
  bool setMode(wifi_mode_t _mode) override;
  wifi_mode_t getMode() override;
  bool setSleepMode(sleep_mode_t type, uint8_t listenInterval = 0) override;
  sleep_mode_t getSleepMode() override;
  bool enableSTA(bool _enable) override;
  bool enableAP(bool _enable) override;
  int hostByName(const char *aHostname, IPAddress &aResult) override;
  int hostByName(const char *aHostname, IPAddress &aResult, uint32_t timeout_ms) override;

  wifi_status_t begin(char *_ssid, char *_passphrase = nullptr, int32_t _channel = 0, const uint8_t *_bssid = nullptr, bool _connect = true) override;
  bool config(IPAddress &_local_ip, IPAddress &_gateway, IPAddress &_subnet) override;
  bool reconnect() override;
  bool disconnect(bool _wifioff = false) override;
  bool isConnected() override;
  bool setAutoConnect(bool _autoConnect) override;
  bool getAutoConnect() override;
  bool setAutoReconnect(bool _autoReconnect) override;
  bool getAutoReconnect() override;
  // int8_t waitForConnectResult(uint32_t _timeoutLength = 60000) override;

  // STA network info
  IPAddress localIP() override;
  uint8_t *macAddress(uint8_t *_mac) override;
  String macAddress() override;
  IPAddress subnetMask() override;
  IPAddress gatewayIP() override;
  IPAddress dnsIP(uint8_t _dns_no = 0) override;

  // STA WiFi info
  wifi_status_t status() override;
  String SSID() const override;
  String psk() const override;
  uint8_t *BSSID() override;
  String BSSIDstr() override;
  int32_t RSSI() override;

  // Soft AP api's
  bool softAP(const char *_ssid, const char *_passphrase = nullptr, int _channel = 1, int _ssid_hidden = 0, int _max_connection = 4) override;
  bool softAPConfig(IPAddress _local_ip, IPAddress _gateway, IPAddress _subnet) override;
  bool softAPdisconnect(bool _wifioff = false) override;
  uint8_t softAPgetStationNum() override;
  IPAddress softAPIP() override;
  uint8_t *softAPmacAddress(uint8_t *_mac) override;
  String softAPmacAddress(void) override;
  String softAPSSID() const override;
  String softAPPSK() const override;

  // n/w scan api's
  int8_t scanNetworks(bool _async = false, bool _show_hidden = false, uint8_t _channel = 0, uint8_t *ssid = nullptr) override;
  void scanNetworksAsync(std::function<void(int)> _onComplete, bool _show_hidden = false) override;
  // int8_t scanComplete() override;
  // void scanDelete() override;
  // uint8_t encryptionType(uint8_t _networkItem) override;
  // bool isHidden(uint8_t _networkItem) override;
  String SSID(uint8_t _networkItem) override;
  int32_t RSSI(uint8_t _networkItem) override;
  uint8_t *BSSID(uint8_t _networkItem) override;
  String BSSIDstr(uint8_t _networkItem) override;
  int32_t channel(uint8_t _networkItem = 0) override;

  // misc api's
  static void preinitWiFiOff();

private:
  /**
   * @var	ESP8266WiFiClass*|&WiFi m_wifi
   */
  ESP8266WiFiClass *m_wifi;
};

#endif
