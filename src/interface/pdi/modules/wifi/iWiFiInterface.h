/****************************** WiFi Interface *********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _I_WIFI_INTERFACE_H_
#define _I_WIFI_INTERFACE_H_

#include <interface/interface_includes.h>

#define WIFI_RSSI_THRESHOLD	-82


// forward declaration of derived class for this interface
class WiFiInterface;
class IPAddress;

/**
 * iWiFiInterface class
 */
class iWiFiInterface
{

public:
  /**
   * iWiFiInterface constructor.
   */
  iWiFiInterface() {}
  /**
   * iWiFiInterface destructor.
   */
  virtual ~iWiFiInterface() {}

  // generic api's
  virtual void setOutputPower(float _dBm) = 0;
  virtual void persistent(bool _persistent) = 0;
  virtual bool setMode(wifi_mode_t _mode) = 0;
  virtual bool setSleepMode(sleep_mode_t type, uint8_t listenInterval = 0) = 0;
  virtual sleep_mode_t getSleepMode() = 0;
  virtual wifi_mode_t getMode() = 0;
  virtual bool enableSTA(bool _enable) = 0;
  virtual bool enableAP(bool _enable) = 0;
  virtual int hostByName(const char *aHostname, IPAddress &aResult, uint32_t timeout_ms) = 0;

  virtual wifi_status_t begin(char *_ssid, char *_passphrase = nullptr, int32_t _channel = 0, const uint8_t *_bssid = nullptr, bool _connect = true) = 0;
  virtual bool config(IPAddress &_local_ip, IPAddress &_gateway, IPAddress &_subnet) = 0;
  virtual bool reconnect() = 0;
  virtual bool disconnect(bool _wifioff = false) = 0;
  virtual bool isConnected() = 0;
  virtual bool setAutoConnect(bool _autoConnect) = 0;
  virtual bool getAutoConnect() = 0;
  virtual bool setAutoReconnect(bool _autoReconnect) = 0;
  virtual bool getAutoReconnect() = 0;

  // STA network info
  virtual IPAddress localIP() = 0;
  virtual uint8_t *macAddress(uint8_t *_mac) = 0;
  virtual String macAddress() = 0;
  virtual IPAddress subnetMask() = 0;
  virtual IPAddress gatewayIP() = 0;
  virtual IPAddress dnsIP(uint8_t _dns_no = 0) = 0;

  // STA WiFi info
  virtual wifi_status_t status() = 0;
  virtual String SSID() const = 0;
  virtual String psk() const = 0;
  virtual uint8_t *BSSID() = 0;
  virtual String BSSIDstr() = 0;
  virtual int32_t RSSI() = 0;

  // Soft AP api's
  virtual bool softAP(const char *_ssid, const char *_passphrase = nullptr, int _channel = 1, int _ssid_hidden = 0, int _max_connection = 4) = 0;
  virtual bool softAPConfig(IPAddress _local_ip, IPAddress _gateway, IPAddress _subnet) = 0;
  virtual bool softAPdisconnect(bool _wifioff = false) = 0;
  virtual uint8_t softAPgetStationNum() = 0;
  virtual IPAddress softAPIP() = 0;
  virtual uint8_t *softAPmacAddress(uint8_t *_mac) = 0;
  virtual String softAPmacAddress(void) = 0;
  virtual String softAPSSID() const = 0;
  virtual String softAPPSK() const = 0;

  // n/w scan api's
  virtual int8_t scanNetworks(bool _async = false, bool _show_hidden = false, uint8_t _channel = 0, uint8_t *ssid = nullptr) = 0;
  virtual void scanNetworksAsync(std::function<void(int)> _onComplete, bool _show_hidden = false) = 0;
  virtual String SSID(uint8_t _networkItem) = 0;
  virtual int32_t RSSI(uint8_t _networkItem) = 0;
  virtual uint8_t *BSSID(uint8_t _networkItem) = 0;
  virtual String BSSIDstr(uint8_t _networkItem) = 0;
  virtual int32_t channel(uint8_t _networkItem = 0) = 0;
  virtual void enableNetworkStatusIndication() = 0;
};

// derived class must define this
extern WiFiInterface __i_wifi;

#endif
