/****************************** WiFi Interface *********************************
This file is part of the pdi stack.

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
  virtual void init() = 0;
  virtual void setOutputPower(float _dBm) = 0;
  virtual void persistent(bool _persistent) = 0;
  virtual bool setMode(wifi_mode_t _mode) = 0;
  virtual bool setSleepMode(sleep_mode_t type, uint8_t listenInterval = 0) = 0;
  virtual sleep_mode_t getSleepMode() = 0;
  virtual wifi_mode_t getMode() = 0;
  virtual bool enableSTA(bool _enable) = 0;
  virtual bool enableAP(bool _enable) = 0;
  virtual uint8_t channel() = 0;
  virtual int hostByName(const char *aHostname, ipaddress_t &aResult, uint32_t timeout_ms) = 0;

  virtual wifi_status_t begin(char *_ssid, char *_passphrase = nullptr, int32_t _channel = 0, const uint8_t *_bssid = nullptr, bool _connect = true) = 0;
  virtual bool config(ipaddress_t &_local_ip, ipaddress_t &_gateway, ipaddress_t &_subnet) = 0;
  virtual bool reconnect() = 0;
  virtual bool disconnect(bool _wifioff = false) = 0;
  virtual bool isConnected() = 0;
  virtual bool setAutoReconnect(bool _autoReconnect) = 0;

  // STA network info
  virtual ipaddress_t localIP() = 0;
  virtual std::string macAddress() = 0;
  virtual ipaddress_t subnetMask() = 0;
  virtual ipaddress_t gatewayIP() = 0;
  virtual ipaddress_t dnsIP(uint8_t _dns_no = 0) = 0;

  // STA WiFi info
  virtual wifi_status_t status() = 0;
  virtual std::string SSID() const = 0;
  virtual uint8_t *BSSID() = 0;
  virtual int32_t RSSI() = 0;

  // Soft AP api's
  virtual bool softAP(const char *_ssid, const char *_passphrase = nullptr, int _channel = 1, int _ssid_hidden = 0, int _max_connection = 4) = 0;
  virtual bool softAPConfig(ipaddress_t _local_ip, ipaddress_t _gateway, ipaddress_t _subnet) = 0;
  virtual bool softAPdisconnect(bool _wifioff = false) = 0;
  virtual ipaddress_t softAPIP() = 0;

  // n/w scan api's
  virtual int8_t scanNetworks(bool _async = false, bool _show_hidden = false, uint8_t _channel = 0, uint8_t *ssid = nullptr) = 0;
  virtual void scanNetworksAsync(std::function<void(int)> _onComplete, bool _show_hidden = false) = 0;
  virtual std::string SSID(uint8_t _networkItem) = 0;
  virtual int32_t RSSI(uint8_t _networkItem) = 0;
  virtual uint8_t *BSSID(uint8_t _networkItem) = 0;
  virtual bool get_bssid_within_scanned_nw_ignoring_connected_stations(char *ssid, uint8_t *bssid, uint8_t *ignorebssid, int _scanCount) = 0;
  virtual bool getApsConnectedStations(std::vector<wifi_station_info_t> &stations) = 0;

  
  virtual void enableNetworkStatusIndication() = 0;
  virtual void enableNAPT() = 0;
};

// derived class must define this
extern WiFiInterface __i_wifi;

#endif
