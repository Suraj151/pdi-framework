/******************************* WiFi Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MOCKDEVICE_WIFI_INTERFACE_H_
#define _MOCKDEVICE_WIFI_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/modules/wifi/iWiFiInterface.h>

/**
 * wifi modes. the framework never names these, so the mock defines its own set
 * rather than borrowing a vendor sdk's.
 */
#define MOCK_WIFI_OFF 0
#define MOCK_WIFI_STA 1
#define MOCK_WIFI_AP 2
#define MOCK_WIFI_AP_STA 3

#ifndef MOCKDEVICE_WIFI_MAX_SCAN_RESULTS
#define MOCKDEVICE_WIFI_MAX_SCAN_RESULTS 8
#endif

/**
 * one entry a scan can report
 */
struct mock_wifi_network_t
{
    char m_ssid[33];
    uint8_t m_bssid[6];
    int32_t m_rssi;
    uint8_t m_channel;

    mock_wifi_network_t();
};

/**
 * @class WiFiInterface
 * @brief A WiFi radio that a test drives instead of a radio driving it.
 *
 * Nothing here talks to hardware. A caller stages the scan results, decides
 * whether an association succeeds, and can drop the link at any point, so the
 * reconnect escalation in WiFiServiceProvider can be exercised without an AP.
 */
class WiFiInterface : public iWiFiInterface
{

public:
  WiFiInterface();
  ~WiFiInterface();

  void init() override;
  void setOutputPower(float _dBm) override;
  void persistent(bool _persistent) override;
  bool setMode(pdi_wifi_mode_t _mode) override;
  bool setSleepMode(sleep_mode_t type, uint8_t listenInterval = 0) override;
  sleep_mode_t getSleepMode() override;
  pdi_wifi_mode_t getMode() override;
  bool enableSTA(bool _enable) override;
  bool enableAP(bool _enable) override;
  uint8_t channel() override;
  int hostByName(const char *aHostname, ipaddress_t &aResult, uint32_t timeout_ms) override;

  wifi_status_t begin(char *_ssid, char *_passphrase = nullptr, int32_t _channel = 0,
                      const uint8_t *_bssid = nullptr, bool _connect = true) override;
  bool config(ipaddress_t &_local_ip, ipaddress_t &_gateway, ipaddress_t &_subnet) override;
  bool reconnect() override;
  bool disconnect(bool _wifioff = false) override;
  bool isConnected() override;
  bool setAutoReconnect(bool _autoReconnect) override;

  ipaddress_t localIP() override;
  pdiutil::string macAddress() override;
  void macAddress(uint8_t *mac) override;
  void setSTAmacAddress(uint8_t *mac) override;
  ipaddress_t subnetMask() override;
  ipaddress_t gatewayIP() override;
  ipaddress_t dnsIP(uint8_t _dns_no = 0) override;

  wifi_status_t status() override;
  pdiutil::string SSID() const override;
  uint8_t *BSSID() override;
  int32_t RSSI() override;

  bool softAP(const char *_ssid, const char *_passphrase = nullptr, int _channel = 1,
              int _ssid_hidden = 0, int _max_connection = 4) override;
  bool softAPConfig(ipaddress_t _local_ip, ipaddress_t _gateway, ipaddress_t _subnet) override;
  bool softAPdisconnect(bool _wifioff = false) override;
  ipaddress_t softAPIP() override;
  pdiutil::string softAPmacAddress() override;
  void softAPmacAddress(uint8_t *mac) override;
  void setSoftAPmacAddress(uint8_t *mac) override;

  int8_t scanNetworks(bool _async = false, bool _show_hidden = false, uint8_t _channel = 0,
                      uint8_t *ssid = nullptr) override;
  void scanNetworksAsync(pdiutil::function<void(int)> _onComplete, bool _show_hidden = false) override;
  pdiutil::string SSID(uint8_t _networkItem) override;
  int32_t RSSI(uint8_t _networkItem) override;
  uint8_t *BSSID(uint8_t _networkItem) override;
  bool get_bssid_within_scanned_nw_ignoring_connected_stations(char *ssid, uint8_t *bssid,
                                                               uint8_t *ignorebssid,
                                                               int _scanCount) override;
  bool getApsConnectedStations(pdiutil::vector<wifi_station_info_t> &stations) override;

  void enableNetworkStatusIndication() override;
  void enableNAPT(bool enable = true) override;

  /**
   * @brief Stage the networks a scan will report. Clears anything staged before.
   */
  void clearScanResults();
  bool addScanResult(const char *ssid, int32_t rssi, uint8_t channel);

  /**
   * @brief Decide whether the next association attempt succeeds.
   */
  void setAssociationAllowed(bool allowed);

  /**
   * @brief Drop the link as though the access point went away.
   */
  void dropConnection();

  /**
   * @brief Address handed out once associated.
   */
  void setStationIp(const ipaddress_t &ip);

  uint32_t getBeginCount() const;
  uint32_t getDisconnectCount() const;
  void clearCounters();

private:
  pdi_wifi_mode_t m_mode;
  sleep_mode_t m_sleepmode;
  bool m_connected;
  bool m_associationallowed;
  bool m_autoreconnect;
  bool m_persistent;
  bool m_apactive;
  uint8_t m_channel;
  uint32_t m_begincount;
  uint32_t m_disconnectcount;

  char m_ssid[33];
  uint8_t m_bssid[6];
  uint8_t m_stamac[6];
  uint8_t m_apmac[6];

  ipaddress_t m_localip;
  ipaddress_t m_gateway;
  ipaddress_t m_subnet;
  ipaddress_t m_dns;
  ipaddress_t m_apip;

  mock_wifi_network_t m_scanresults[MOCKDEVICE_WIFI_MAX_SCAN_RESULTS];
  uint8_t m_scancount;
};

#endif // _MOCKDEVICE_WIFI_INTERFACE_H_
