/**************************** WiFi Interface ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "WiFiInterface.h"
#include "LoggerInterface.h"
#include "DeviceControlInterface.h"

#if defined( ENABLE_NAPT_FEATURE )
#include "lwip/lwip_napt.h"
#include "lwip/app/dhcpserver.h"
#elif defined( ENABLE_NAPT_FEATURE_LWIP_V2 )
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <LwipDhcpServer-NonOS.h>
#endif

extern "C" void preinit() {
#ifdef ENABLE_NAPT_FEATURE_LWIP_V2
  WiFiInterface::preinitWiFiOff();
#endif
  __i_dvc_ctrl.eraseConfig();
	uint8_t sta_mac[6];
  wifi_get_macaddr(STATION_IF, sta_mac);
  sta_mac[0] +=2;
	wifi_set_macaddr(SOFTAP_IF, sta_mac);
}

/**
 * begin wifi interface
 */
wifi_status_t toWifiStatus(wl_status_t stat)
{
  wifi_status_t _wifi_status = CONN_STATUS_MAX;
  switch (stat)
  {
  case WL_CONNECTED:
    _wifi_status = CONN_STATUS_CONNECTED;
    break;    
  case WL_NO_SSID_AVAIL:
    _wifi_status = CONN_STATUS_NOT_AVAILABLE;
    break;
  case WL_CONNECT_FAILED:
    _wifi_status = CONN_STATUS_CONNECTION_FAILED;
    break;
  case WL_WRONG_PASSWORD:
    _wifi_status = CONN_STATUS_CONFIG_ERROR;
    break;
  case WL_DISCONNECTED:
    _wifi_status = CONN_STATUS_DISCONNECTED;
    break;
  default:
    _wifi_status = CONN_STATUS_MAX;
    break;
  }

  return _wifi_status;
}

/**
 * WiFiInterface constructor.
 */
WiFiInterface::WiFiInterface() : m_wifi(&WiFi), m_wifi_led(0)
{
  wifi_set_event_handler_cb( (wifi_event_handler_cb_t)&WiFiInterface::wifi_event_handler_cb );
}

/**
 * WiFiInterface destructor.
 */
WiFiInterface::~WiFiInterface()
{
  this->m_wifi = nullptr;
}

/**
 * Init the wifi interface
 */
void WiFiInterface::init()
{
  // this->m_wifi->mode(WIFI_AP_STA);
  setSleepMode(WIFI_NONE_SLEEP);  // WIFI_NONE_SLEEP = 0, WIFI_LIGHT_SLEEP = 1, WIFI_MODEM_SLEEP = 2
  setOutputPower(21.0);  // dBm max: +20.5dBm  min: 0dBm
  persistent(false);
}

/**
 * setOutputPower
 */
void WiFiInterface::setOutputPower(float _dBm)
{
  if (nullptr != this->m_wifi)
  {
    this->m_wifi->setOutputPower(_dBm);
  }
}

/**
 * set persistent
 */
void WiFiInterface::persistent(bool _persistent)
{
  if (nullptr != this->m_wifi)
  {
    this->m_wifi->persistent(_persistent);
  }
}

/**
 * setMode
 */
bool WiFiInterface::setMode(pdi_wifi_mode_t _mode)
{

  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->mode(static_cast<WiFiMode_t>(_mode));
  }
  return status;
}

/**
 * getMode
 */
pdi_wifi_mode_t WiFiInterface::getMode()
{

  pdi_wifi_mode_t mode = 0;
  if (nullptr != this->m_wifi)
  {
    mode = static_cast<pdi_wifi_mode_t>(this->m_wifi->getMode());
  }
  return mode;
}

/**
 * setSleepMode
 */
bool WiFiInterface::setSleepMode(sleep_mode_t type, uint8_t listenInterval)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->setSleepMode(static_cast<WiFiSleepType_t>(type), listenInterval);
  }
  return status;
}

/**
 * getSleepMode
 */
sleep_mode_t WiFiInterface::getSleepMode()
{

  sleep_mode_t mode = 0;
  if (nullptr != this->m_wifi)
  {
    mode = this->m_wifi->getSleepMode();
  }
  return mode;
}

/**
 * enableSTA
 */
bool WiFiInterface::enableSTA(bool _enable)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->enableSTA(_enable);
  }
  return status;
}

/**
 * enableAP
 */
bool WiFiInterface::enableAP(bool _enable)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->enableAP(_enable);
  }
  return status;
}

/**
 * return the channel
 */
uint8_t WiFiInterface::channel()
{
  uint8_t channel = 0;
  if (nullptr != this->m_wifi)
  {
    channel = this->m_wifi->channel();
  }
  return channel;
}

/**
 * hostByName
 */
int WiFiInterface::hostByName(const char *aHostname, ipaddress_t &aResult, uint32_t timeout_ms)
{
  int status = 0;
  if (nullptr != this->m_wifi)
  {
    IPAddress ip = (uint32_t)aResult;
    status = this->m_wifi->hostByName(aHostname, ip, timeout_ms);
    aResult = (uint32_t)ip;
  }
  return status;
}

/**
 * begin wifi interface
 */
wifi_status_t WiFiInterface::begin(char *_ssid, char *_passphrase, int32_t _channel, const uint8_t *_bssid, bool _connect)
{
  wifi_status_t _wifi_status = CONN_STATUS_MAX;
  if (nullptr != this->m_wifi)
  {
    wl_status_t stat = this->m_wifi->begin(_ssid, _passphrase, _channel, _bssid, _connect);
    _wifi_status = toWifiStatus(stat);
  }
  return _wifi_status;
}

/**
 * config
 */
bool WiFiInterface::config(ipaddress_t &_local_ip, ipaddress_t &_gateway, ipaddress_t &_subnet)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->config((uint32_t)_local_ip, (uint32_t)_gateway, (uint32_t)_subnet);
  }
  return status;
}

/**
 * reconnect
 */
bool WiFiInterface::reconnect()
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->reconnect();
  }
  return status;
}

/**
 * disconnect
 */
bool WiFiInterface::disconnect(bool _wifioff)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->disconnect(_wifioff);
  }
  return status;
}

/**
 * isConnected
 */
bool WiFiInterface::isConnected()
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->isConnected();
  }
  return status;
}

/**
 * setAutoReconnect
 */
bool WiFiInterface::setAutoReconnect(bool _autoReconnect)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->setAutoReconnect(_autoReconnect);
  }
  return status;
}

/**
 * localIP
 */
ipaddress_t WiFiInterface::localIP()
{
  IPAddress ip(0);
  if (nullptr != this->m_wifi)
  {
    ip = this->m_wifi->localIP();
  }
  return (uint32_t)ip;
}

/**
 * macAddress
 */
pdiutil::string WiFiInterface::macAddress()
{
  pdiutil::string mac;
  if (nullptr != this->m_wifi)
  {
    mac = this->m_wifi->macAddress().c_str();
  }
  return mac;
}

/**
 * subnetMask
 */
ipaddress_t WiFiInterface::subnetMask()
{
  IPAddress ip(0);
  if (nullptr != this->m_wifi)
  {
    ip = this->m_wifi->subnetMask();
  }
  return (uint32_t)ip;
}

/**
 * gatewayIP
 */
ipaddress_t WiFiInterface::gatewayIP()
{
  IPAddress ip(0);
  if (nullptr != this->m_wifi)
  {
    ip = this->m_wifi->gatewayIP();
  }
  return (uint32_t)ip;
}

/**
 * dnsIP
 */
ipaddress_t WiFiInterface::dnsIP(uint8_t _dns_no)
{
  IPAddress ip(0);
  if (nullptr != this->m_wifi)
  {
    ip = this->m_wifi->dnsIP(_dns_no);
  }
  return (uint32_t)ip;
}

/**
 * status
 */
wifi_status_t WiFiInterface::status()
{
  wifi_status_t _wifi_status = CONN_STATUS_MAX;
  if (nullptr != this->m_wifi)
  {
    wl_status_t stat = this->m_wifi->status();
    _wifi_status = toWifiStatus(stat);
  }
  return _wifi_status;
}

/**
 * SSID
 */
pdiutil::string WiFiInterface::SSID() const
{
  pdiutil::string ssid;
  if (nullptr != this->m_wifi)
  {
    ssid = this->m_wifi->SSID().c_str();
  }
  return ssid;
}

/**
 * BSSID
 */
uint8_t *WiFiInterface::BSSID()
{
  uint8_t *bssid = nullptr;
  if (nullptr != this->m_wifi)
  {
    bssid = this->m_wifi->BSSID();
  }
  return bssid;
}

/**
 * RSSI
 */
int32_t WiFiInterface::RSSI()
{
  int32_t rssi = 0;
  if (nullptr != this->m_wifi)
  {
    rssi = this->m_wifi->RSSI();
  }
  return rssi;
}

/**
 * softAP
 */
bool WiFiInterface::softAP(const char *_ssid, const char *_passphrase, int _channel, int _ssid_hidden, int _max_connection)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->softAP(_ssid, _passphrase, _channel, _ssid_hidden, _max_connection);
  }
  return status;
}

/**
 * softAPConfig
 */
bool WiFiInterface::softAPConfig(ipaddress_t _local_ip, ipaddress_t _gateway, ipaddress_t _subnet)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->softAPConfig((uint32_t)_local_ip, (uint32_t)_gateway, (uint32_t)_subnet);
  }
  return status;
}

/**
 * softAPdisconnect
 */
bool WiFiInterface::softAPdisconnect(bool _wifioff)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->softAPdisconnect(_wifioff);
  }
  return status;
}

/**
 * softAPIP
 */
ipaddress_t WiFiInterface::softAPIP()
{
  IPAddress ip(0);
  if (nullptr != this->m_wifi)
  {
    ip = this->m_wifi->softAPIP();
  }
  return (uint32_t)ip;
}

/**
 * scanNetworks
 */
int8_t WiFiInterface::scanNetworks(bool _async, bool _show_hidden, uint8_t _channel, uint8_t *ssid)
{
  int8_t num = 0;
  if (nullptr != this->m_wifi)
  {
    num = this->m_wifi->scanNetworks(_async, _show_hidden, _channel, ssid);
  }
  return num;
}

/**
 * scanNetworksAsync
 */
void WiFiInterface::scanNetworksAsync(pdiutil::function<void(int)> _onComplete, bool _show_hidden)
{
  if (nullptr != this->m_wifi)
  {
    this->m_wifi->scanNetworksAsync(_onComplete, _show_hidden);
  }
}

/**
 * SSID
 */
pdiutil::string WiFiInterface::SSID(uint8_t _networkItem)
{
  pdiutil::string ssid;
  if (nullptr != this->m_wifi)
  {
    ssid = this->m_wifi->SSID(_networkItem).c_str();
  }
  return ssid;
}

/**
 * RSSI
 */
int32_t WiFiInterface::RSSI(uint8_t _networkItem)
{
  int32_t rssi = 0;
  if (nullptr != this->m_wifi)
  {
    rssi = this->m_wifi->RSSI(_networkItem);
  }
  return rssi;
}

/**
 * BSSID
 */
uint8_t *WiFiInterface::BSSID(uint8_t _networkItem)
{
  uint8_t *bssid = nullptr;
  if (nullptr != this->m_wifi)
  {
    bssid = this->m_wifi->BSSID(_networkItem);
  }
  return bssid;
}

/**
 * scan ssid not in connected stations and return bssid and found result
 * 1. Get connected stations list
 * 2. Check whether scan result items are already in connected list
 * 3. If scanned ssid not in connected stations list then it can be connected
 * 4. Check its bssid marked as not ignored before make confimation on connection
 *
 * @param   char*     ssid
 * @param   uint8_t*  bssid
 * @return  bool
 */
bool WiFiInterface::get_bssid_within_scanned_nw_ignoring_connected_stations(char *ssid, uint8_t *bssid, uint8_t *ignorebssid, int _scanCount)
{

  if( nullptr == this->m_wifi || nullptr == ssid || nullptr == bssid || nullptr == ignorebssid ){
    return false;
  }

  LogI("Scanning stations\n");

  int n = _scanCount;
  // int indices[n];
  // for (int i = 0; i < n; i++) {
  //   indices[i] = i;
  // }
  // for (int i = 0; i < n; i++) {
  //   for (int j = i + 1; j < n; j++) {
  //     if (this->m_wifi->RSSI(indices[j]) > this->m_wifi->RSSI(indices[i])) {
  //       std::swap(indices[i], indices[j]);
  //     }
  //   }
  // }
  struct station_info * stat_info = wifi_softap_get_station_info();
  struct station_info * stat_info_copy = stat_info;
  char* _ssid_buff = new char[30];

  if( nullptr == _ssid_buff ){
    return false;
  }
  memset( _ssid_buff, 0, 30 );

  for (int i = 0; i < n; ++i) {

    pdiutil::string _ssid = this->SSID(i);
    // _ssid.toCharArray( _ssid_buff, _ssid.length()+1 );
    strncpy(_ssid_buff, _ssid.c_str(), _ssid.size());

    if( __are_arrays_equal( ssid, _ssid_buff, _ssid.size() ) ){

      bool _found = false;
      stat_info = stat_info_copy;
      while ( nullptr != stat_info ) {

        memcpy(bssid, stat_info->bssid, 6);
        bssid[0] +=2;

        if( __are_arrays_equal( (char*)bssid, (char*)this->BSSID(i), 6 ) ){

          _found = true;
          break;
        }

        stat_info = STAILQ_NEXT(stat_info, next);
      }

      if( !_found ){

        if( !__are_arrays_equal( (char*)ignorebssid, (char*)this->BSSID(i), 6 ) ){
          memcpy(bssid, this->BSSID(i), 6);
          delete[] _ssid_buff;
          return true;
        }
      }
    }
  }

  delete[] _ssid_buff;
  return false;
}

/**
 * return the list of connected stations info to acess point
 */
bool WiFiInterface::getApsConnectedStations(pdiutil::vector<wifi_station_info_t> &stations)
{
  struct station_info *stat_info = wifi_softap_get_station_info();

  while (nullptr != stat_info)
  {
    wifi_station_info_t _station(stat_info->bssid, stat_info->ip.addr);
    stations.push_back(_station);
    stat_info = STAILQ_NEXT(stat_info, next);
  }

  return true;
}

/**
 * n/w status indication
 */
void WiFiInterface::enableNetworkStatusIndication()
{
  m_wifi_led = 2;

  __i_dvc_ctrl.gpioMode(DIGITAL_WRITE, this->m_wifi_led);
  __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, this->m_wifi_led, HIGH );

  __task_scheduler.setInterval( [&]() { 

      LogI("Handling LED Status Indications\n");
      LogFmtI("RSSI : %d\n", this->m_wifi->RSSI());

      if( !this->m_wifi->localIP().isSet() || !this->m_wifi->isConnected() || ( this->m_wifi->RSSI() < (int)WIFI_RSSI_THRESHOLD ) ){

        LogW("WiFi not connected.\n");
        __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, this->m_wifi_led, LOW );
      }else{

        __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, this->m_wifi_led, HIGH );
        __i_dvc_ctrl.wait(40);
        __i_dvc_ctrl.gpioWrite(DIGITAL_WRITE, this->m_wifi_led, LOW );
      }

    }, 2.5*MILLISECOND_DURATION_1000, __i_dvc_ctrl.millis_now() );
}

/**
 * enable napt feature
 */
void WiFiInterface::enableNAPT(bool enable)
{
#if defined( ENABLE_NAPT_FEATURE )
  // Initialize the NAT feature
  ip_napt_init(IP_NAPT_MAX, IP_PORTMAP_MAX);
  // Enable NAT on the AP interface
  ip_napt_enable_no(1, 1);
  // Set the DNS server for clients of the AP to the one we also use for the STA interface
  IPAddress _ip((uint32_t)__i_wifi.dnsIP());
  dhcps_set_DNS(_ip);
  LogFmtS("NAPT(lwip %d) initialization done\n", (int)LWIP_VERSION_MAJOR);
#elif defined( ENABLE_NAPT_FEATURE_LWIP_V2 )
  // Initialize the NAPT feature
  err_t ret = ip_napt_init(IP_NAPT_MAX, IP_PORTMAP_MAX);
  if (ret == ERR_OK) {
    // Enable NAT on the AP interface
    ret = ip_napt_enable_no(SOFTAP_IF, 1);
    if (ret == ERR_OK) {
      LogFmtS("NAPT(lwip %d) initialization done\n", (int)LWIP_VERSION_MAJOR);
      // Set the DNS server for clients of the AP to the one we also use for the STA interface
      IPAddress _ip((uint32_t)__i_wifi.dnsIP(0));
      getNonOSDhcpServer().setDns(_ip);
      //dhcpSoftAP.dhcps_set_dns(1, __i_wifi.dnsIP(1));
    }
  }
  if (ret != ERR_OK) {
    LogE("NAPT initialization failed\n");
  }
#endif
}

/**
 * static wifi event handler
 *
 */
void WiFiInterface::wifi_event_handler_cb(System_Event_t *_event)
{
  if( nullptr != _event ){

    LogFmtI("\nwifi event : %d\n", (int)_event->event);
    event_name_t e = EVENT_NAME_MAX;

    if ( EVENT_STAMODE_CONNECTED == _event->event ) {
      __task_scheduler.setTimeout( [&]() { __i_wifi.enableNAPT(); }, NAPT_INIT_DURATION_AFTER_WIFI_CONNECT, __i_dvc_ctrl.millis_now() );
      e = EVENT_WIFI_STA_CONNECTED;
    }else if( EVENT_STAMODE_DISCONNECTED == _event->event ){
      e = EVENT_WIFI_STA_DISCONNECTED;
    }else if( EVENT_SOFTAPMODE_STACONNECTED == _event->event ){
      e = EVENT_WIFI_AP_STACONNECTED;
    }else if( EVENT_SOFTAPMODE_STADISCONNECTED == _event->event ){
      e = EVENT_WIFI_AP_STADISCONNECTED;
    }

    if( EVENT_NAME_MAX != e ){
      __utl_event.execute_event(e, _event);
    }
  }
}

/**
 * preinitWiFiOff
 */
void WiFiInterface::preinitWiFiOff()
{
  ESP8266WiFiClass::preinitWiFiOff();
}

WiFiInterface __i_wifi;
