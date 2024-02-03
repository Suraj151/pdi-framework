/**************************** WiFi Interface ***********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "WiFiInterface.h"

/**
 * WiFiInterface constructor.
 */
WiFiInterface::WiFiInterface() : m_wifi(&WiFi)
{
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
 * get Persistent
 */
bool WiFiInterface::getPersistent()
{
  bool persistent = false;
  if (nullptr != this->m_wifi)
  {
    persistent = this->m_wifi->getPersistent();
  }
  return persistent;
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
int WiFiInterface::hostByName(const char *aHostname, IPAddress &aResult)
{
  int status = 0;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->hostByName(aHostname, aResult);
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
 * setAutoConnect
 */
bool WiFiInterface::setAutoConnect(bool _autoConnect)
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->setAutoConnect(_autoConnect);
  }
  return status;
}

/**
 * getAutoConnect
 */
bool WiFiInterface::getAutoConnect()
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->getAutoConnect();
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
 * getAutoReconnect
 */
bool WiFiInterface::getAutoReconnect()
{
  bool status = false;
  if (nullptr != this->m_wifi)
  {
    status = this->m_wifi->getAutoReconnect();
  }
  return status;
}

// /**
//  * waitForConnectResult
//  */
// int8_t WiFiInterface::waitForConnectResult(uint32_t _timeoutLength){
//   int8_t result = false;
//   if( nullptr != this->m_wifi ){
//     result = this->m_wifi->waitForConnectResult(_timeoutLength);
//   }
//   return result;
// }

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
uint8_t *WiFiInterface::macAddress(uint8_t *_mac)
{
  uint8_t *mac = nullptr;
  if (nullptr != this->m_wifi)
  {
    mac = this->m_wifi->macAddress(_mac);
  }
  return mac;
}

/**
 * macAddress
 */
String WiFiInterface::macAddress()
{
  String mac;
  if (nullptr != this->m_wifi)
  {
    mac = this->m_wifi->macAddress();
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
String WiFiInterface::SSID() const
{
  String ssid;
  if (nullptr != this->m_wifi)
  {
    ssid = this->m_wifi->SSID();
  }
  return ssid;
}

/**
 * psk
 */
String WiFiInterface::psk() const
{
  String psk;
  if (nullptr != this->m_wifi)
  {
    psk = this->m_wifi->psk();
  }
  return psk;
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
 * BSSIDstr
 */
String WiFiInterface::BSSIDstr()
{
  String bssid_str;
  if (nullptr != this->m_wifi)
  {
    bssid_str = this->m_wifi->BSSIDstr();
  }
  return bssid_str;
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
 * softAPgetStationNum
 */
uint8_t WiFiInterface::softAPgetStationNum()
{
  uint8_t sta_num = 0;
  if (nullptr != this->m_wifi)
  {
    sta_num = this->m_wifi->softAPgetStationNum();
  }
  return sta_num;
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
 * softAPmacAddress
 */
uint8_t *WiFiInterface::softAPmacAddress(uint8_t *_mac)
{
  uint8_t *mac = nullptr;
  if (nullptr != this->m_wifi)
  {
    mac = this->m_wifi->softAPmacAddress(_mac);
  }
  return mac;
}

/**
 * softAPmacAddress
 */
String WiFiInterface::softAPmacAddress(void)
{
  String mac;
  if (nullptr != this->m_wifi)
  {
    mac = this->m_wifi->softAPmacAddress();
  }
  return mac;
}

/**
 * softAPSSID
 */
String WiFiInterface::softAPSSID() const
{
  String ssid;
  if (nullptr != this->m_wifi)
  {
    ssid = this->m_wifi->softAPSSID();
  }
  return ssid;
}

/**
 * softAPPSK
 */
String WiFiInterface::softAPPSK() const
{
  String psk;
  if (nullptr != this->m_wifi)
  {
    psk = this->m_wifi->softAPPSK();
  }
  return psk;
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

// /**
//  * scanComplete
//  */
// int8_t WiFiInterface::scanComplete(){
//   int8_t num = 0;
//   if( nullptr != this->m_wifi ){
//     num = this->m_wifi->scanComplete();
//   }
//   return num;
// }
//
// /**
//  * scanDelete
//  */
// void WiFiInterface::scanDelete(){
//   if( nullptr != this->m_wifi ){
//     this->m_wifi->scanDelete();
//   }
// }
//
// /**
//  * encryptionType
//  */
// uint8_t WiFiInterface::encryptionType(uint8_t _networkItem){
//   uint8_t enc_type = 0;
//   if( nullptr != this->m_wifi ){
//     enc_type = this->m_wifi->encryptionType(_networkItem);
//   }
//   return enc_type;
// }
// /**
//  * isHidden
//  */
// bool WiFiInterface::isHidden(uint8_t _networkItem){
//   bool is_hidden = false;
//   if( nullptr != this->m_wifi ){
//     is_hidden = this->m_wifi->isHidden(_networkItem);
//   }
//   return is_hidden;
// }

/**
 * SSID
 */
String WiFiInterface::SSID(uint8_t _networkItem)
{
  String ssid;
  if (nullptr != this->m_wifi)
  {
    ssid = this->m_wifi->SSID(_networkItem);
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
 * BSSIDstr
 */
String WiFiInterface::BSSIDstr(uint8_t _networkItem)
{
  String bssid;
  if (nullptr != this->m_wifi)
  {
    bssid = this->m_wifi->BSSIDstr(_networkItem);
  }
  return bssid;
}

/**
 * channel
 */
int32_t WiFiInterface::channel(uint8_t _networkItem)
{
  int32_t channel = 0;
  if (nullptr != this->m_wifi)
  {
    channel = this->m_wifi->channel();
  }
  return channel;
}

/**
 * preinitWiFiOff
 */
void WiFiInterface::preinitWiFiOff()
{
  ESP8266WiFiClass::preinitWiFiOff();
}

WiFiInterface __i_wifi;
