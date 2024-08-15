/**************************** WiFi Interface ***********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "WiFiInterface.h"
#if defined( ENABLE_NAPT_FEATURE )
#include "lwip/lwip_napt.h"
#include "lwip/app/dhcpserver.h"
#elif defined( ENABLE_NAPT_FEATURE_LWIP_V2 )
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <LwipDhcpServer-NonOS.h>
#endif

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
bool WiFiInterface::setMode(wifi_mode_t _mode)
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
wifi_mode_t WiFiInterface::getMode()
{

  wifi_mode_t mode = 0;
  if (nullptr != this->m_wifi)
  {
    mode = static_cast<wifi_mode_t>(this->m_wifi->getMode());
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
 * hostByName
 */
int WiFiInterface::hostByName(const char *aHostname, IPAddress &aResult, uint32_t timeout_ms)
{
  int status = 0;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->hostByName(aHostname, aResult, timeout_ms);
  }
  return status;
}

/**
 * begin
 */
wifi_status_t WiFiInterface::begin(char *_ssid, char *_passphrase, int32_t _channel, const uint8_t *_bssid, bool _connect)
{
  wifi_status_t _wifi_status = 0;
  if (nullptr != this->m_wifi)
  {
    _wifi_status = static_cast<wifi_status_t>(this->m_wifi->begin(_ssid, _passphrase, _channel, _bssid, _connect));
  }
  return _wifi_status;
}

/**
 * config
 */
bool WiFiInterface::config(IPAddress &_local_ip, IPAddress &_gateway, IPAddress &_subnet)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->config(_local_ip, _gateway, _subnet);
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
IPAddress WiFiInterface::localIP()
{
  IPAddress ip(0);
  if (nullptr != this->m_wifi)
  {
    ip = this->m_wifi->localIP();
  }
  return ip;
}

/**
 * macAddress
 */
std::string WiFiInterface::macAddress()
{
  std::string mac;
  if (nullptr != this->m_wifi)
  {
    mac = this->m_wifi->macAddress().c_str();
  }
  return mac;
}

/**
 * subnetMask
 */
IPAddress WiFiInterface::subnetMask()
{
  IPAddress ip(0);
  if (nullptr != this->m_wifi)
  {
    ip = this->m_wifi->subnetMask();
  }
  return ip;
}

/**
 * gatewayIP
 */
IPAddress WiFiInterface::gatewayIP()
{
  IPAddress ip(0);
  if (nullptr != this->m_wifi)
  {
    ip = this->m_wifi->gatewayIP();
  }
  return ip;
}

/**
 * dnsIP
 */
IPAddress WiFiInterface::dnsIP(uint8_t _dns_no)
{
  IPAddress ip(0);
  if (nullptr != this->m_wifi)
  {
    ip = this->m_wifi->dnsIP(_dns_no);
  }
  return ip;
}

/**
 * status
 */
wifi_status_t WiFiInterface::status()
{
  wifi_status_t _wifi_status = 0;
  if (nullptr != this->m_wifi)
  {
    _wifi_status = static_cast<wifi_status_t>(this->m_wifi->status());
  }
  return _wifi_status;
}

/**
 * SSID
 */
std::string WiFiInterface::SSID() const
{
  std::string ssid;
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
bool WiFiInterface::softAPConfig(IPAddress _local_ip, IPAddress _gateway, IPAddress _subnet)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->softAPConfig(_local_ip, _gateway, _subnet);
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
IPAddress WiFiInterface::softAPIP()
{
  IPAddress ip(0);
  if (nullptr != this->m_wifi)
  {
    ip = this->m_wifi->softAPIP();
  }
  return ip;
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
void WiFiInterface::scanNetworksAsync(std::function<void(int)> _onComplete, bool _show_hidden)
{
  if (nullptr != this->m_wifi)
  {
    this->m_wifi->scanNetworksAsync(_onComplete, _show_hidden);
  }
}

/**
 * SSID
 */
std::string WiFiInterface::SSID(uint8_t _networkItem)
{
  std::string ssid;
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
 * n/w status indication
 */
void WiFiInterface::enableNetworkStatusIndication()
{
  m_wifi_led = 4;

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
void WiFiInterface::enableNAPT()
{
#if defined( ENABLE_NAPT_FEATURE )
  // Initialize the NAT feature
  ip_napt_init(IP_NAPT_MAX, IP_PORTMAP_MAX);
  // Enable NAT on the AP interface
  ip_napt_enable_no(1, 1);
  // Set the DNS server for clients of the AP to the one we also use for the STA interface
  dhcps_set_DNS(__i_wifi.dnsIP());
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
      getNonOSDhcpServer().setDns(__i_wifi.dnsIP(0));
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
