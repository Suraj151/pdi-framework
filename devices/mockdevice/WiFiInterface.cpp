/******************************* WiFi Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "WiFiInterface.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

mock_wifi_network_t::mock_wifi_network_t() : m_rssi(0), m_channel(0)
{
    memset(m_ssid, 0, sizeof(m_ssid));
    memset(m_bssid, 0, sizeof(m_bssid));
}

static void formatMac(const uint8_t *mac, char *out)
{
    __snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4],
               mac[5]);
}

WiFiInterface::WiFiInterface() : m_mode(MOCK_WIFI_OFF),
                                 m_sleepmode(0),
                                 m_connected(false),
                                 m_associationallowed(true),
                                 m_autoreconnect(true),
                                 m_persistent(false),
                                 m_apactive(false),
                                 m_channel(1),
                                 m_begincount(0),
                                 m_disconnectcount(0),
                                 m_scancount(0)
{
    memset(m_ssid, 0, sizeof(m_ssid));

    const uint8_t stamac[6] = {0x02, 0x00, 0x00, 0xAB, 0xCD, 0xEF};
    const uint8_t apmac[6] = {0x02, 0x00, 0x00, 0xAB, 0xCD, 0xF0};
    const uint8_t bssid[6] = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    memcpy(m_stamac, stamac, 6);
    memcpy(m_apmac, apmac, 6);
    memcpy(m_bssid, bssid, 6);

    m_localip = ipaddress_t(192, 168, 1, 50);
    m_gateway = ipaddress_t(192, 168, 1, 1);
    m_subnet = ipaddress_t(255, 255, 255, 0);
    m_dns = ipaddress_t(192, 168, 1, 1);
    m_apip = ipaddress_t(192, 168, 0, 1);
}

WiFiInterface::~WiFiInterface()
{
}

void WiFiInterface::init()
{
}

void WiFiInterface::setOutputPower(float _dBm)
{
}

void WiFiInterface::persistent(bool _persistent)
{
    m_persistent = _persistent;
}

bool WiFiInterface::setMode(pdi_wifi_mode_t _mode)
{
    m_mode = _mode;
    return true;
}

bool WiFiInterface::setSleepMode(sleep_mode_t type, uint8_t listenInterval)
{
    m_sleepmode = type;
    return true;
}

sleep_mode_t WiFiInterface::getSleepMode()
{
    return m_sleepmode;
}

pdi_wifi_mode_t WiFiInterface::getMode()
{
    return m_mode;
}

bool WiFiInterface::enableSTA(bool _enable)
{
    if (_enable)
    {
        m_mode = m_apactive ? MOCK_WIFI_AP_STA : MOCK_WIFI_STA;
    }
    else
    {
        m_connected = false;
        m_mode = m_apactive ? MOCK_WIFI_AP : MOCK_WIFI_OFF;
    }
    return true;
}

bool WiFiInterface::enableAP(bool _enable)
{
    m_apactive = _enable;
    if (_enable)
    {
        m_mode = (MOCK_WIFI_STA == m_mode || MOCK_WIFI_AP_STA == m_mode) ? MOCK_WIFI_AP_STA
                                                                        : MOCK_WIFI_AP;
    }
    else
    {
        m_mode = (MOCK_WIFI_AP_STA == m_mode) ? MOCK_WIFI_STA : MOCK_WIFI_OFF;
    }
    return true;
}

uint8_t WiFiInterface::channel()
{
    return m_channel;
}

/**
 * resolves through the host resolver, so a name lookup behaves like one
 */
int WiFiInterface::hostByName(const char *aHostname, ipaddress_t &aResult, uint32_t timeout_ms)
{
    if (nullptr == aHostname)
    {
        return 0;
    }

    struct addrinfo hints;
    struct addrinfo *res = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (0 != getaddrinfo(aHostname, nullptr, &hints, &res) || nullptr == res)
    {
        return 0;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    uint32_t raw = ntohl(addr->sin_addr.s_addr);
    aResult = ipaddress_t((uint8_t)((raw >> 24) & 0xFF), (uint8_t)((raw >> 16) & 0xFF),
                          (uint8_t)((raw >> 8) & 0xFF), (uint8_t)(raw & 0xFF));

    freeaddrinfo(res);
    return 1;
}

wifi_status_t WiFiInterface::begin(char *_ssid, char *_passphrase, int32_t _channel,
                                   const uint8_t *_bssid, bool _connect)
{
    m_begincount++;

    if (nullptr != _ssid)
    {
        strncpy(m_ssid, _ssid, sizeof(m_ssid) - 1);
        m_ssid[sizeof(m_ssid) - 1] = '\0';
    }

    if (nullptr != _bssid)
    {
        memcpy(m_bssid, _bssid, 6);
    }

    if (_channel > 0)
    {
        m_channel = (uint8_t)_channel;
    }

    if (!_connect || !m_associationallowed)
    {
        m_connected = false;
        return CONN_STATUS_CONNECTION_FAILED;
    }

    m_connected = true;
    return CONN_STATUS_CONNECTED;
}

bool WiFiInterface::config(ipaddress_t &_local_ip, ipaddress_t &_gateway, ipaddress_t &_subnet)
{
    m_localip = _local_ip;
    m_gateway = _gateway;
    m_subnet = _subnet;
    return true;
}

bool WiFiInterface::reconnect()
{
    if (!m_associationallowed)
    {
        m_connected = false;
        return false;
    }

    m_connected = true;
    return true;
}

bool WiFiInterface::disconnect(bool _wifioff)
{
    m_disconnectcount++;
    m_connected = false;
    if (_wifioff)
    {
        m_mode = MOCK_WIFI_OFF;
    }
    return true;
}

bool WiFiInterface::isConnected()
{
    return m_connected;
}

bool WiFiInterface::setAutoReconnect(bool _autoReconnect)
{
    m_autoreconnect = _autoReconnect;
    return true;
}

ipaddress_t WiFiInterface::localIP()
{
    return m_connected ? m_localip : ipaddress_t();
}

pdiutil::string WiFiInterface::macAddress()
{
    char buf[18];
    memset(buf, 0, sizeof(buf));
    formatMac(m_stamac, buf);
    return pdiutil::string(buf);
}

void WiFiInterface::macAddress(uint8_t *mac)
{
    if (nullptr != mac)
    {
        memcpy(mac, m_stamac, 6);
    }
}

void WiFiInterface::setSTAmacAddress(uint8_t *mac)
{
    if (nullptr != mac)
    {
        memcpy(m_stamac, mac, 6);
    }
}

ipaddress_t WiFiInterface::subnetMask()
{
    return m_subnet;
}

ipaddress_t WiFiInterface::gatewayIP()
{
    return m_gateway;
}

ipaddress_t WiFiInterface::dnsIP(uint8_t _dns_no)
{
    return m_dns;
}

wifi_status_t WiFiInterface::status()
{
    return m_connected ? CONN_STATUS_CONNECTED : CONN_STATUS_DISCONNECTED;
}

pdiutil::string WiFiInterface::SSID() const
{
    return pdiutil::string(m_ssid);
}

uint8_t *WiFiInterface::BSSID()
{
    return m_bssid;
}

int32_t WiFiInterface::RSSI()
{
    return m_connected ? -55 : 0;
}

bool WiFiInterface::softAP(const char *_ssid, const char *_passphrase, int _channel,
                           int _ssid_hidden, int _max_connection)
{
    m_apactive = true;
    m_mode = (MOCK_WIFI_STA == m_mode || MOCK_WIFI_AP_STA == m_mode) ? MOCK_WIFI_AP_STA
                                                                    : MOCK_WIFI_AP;
    if (_channel > 0)
    {
        m_channel = (uint8_t)_channel;
    }
    return true;
}

bool WiFiInterface::softAPConfig(ipaddress_t _local_ip, ipaddress_t _gateway, ipaddress_t _subnet)
{
    m_apip = _local_ip;
    return true;
}

bool WiFiInterface::softAPdisconnect(bool _wifioff)
{
    m_apactive = false;
    m_mode = (MOCK_WIFI_AP_STA == m_mode) ? MOCK_WIFI_STA : MOCK_WIFI_OFF;
    return true;
}

ipaddress_t WiFiInterface::softAPIP()
{
    return m_apip;
}

pdiutil::string WiFiInterface::softAPmacAddress()
{
    char buf[18];
    memset(buf, 0, sizeof(buf));
    formatMac(m_apmac, buf);
    return pdiutil::string(buf);
}

void WiFiInterface::softAPmacAddress(uint8_t *mac)
{
    if (nullptr != mac)
    {
        memcpy(mac, m_apmac, 6);
    }
}

void WiFiInterface::setSoftAPmacAddress(uint8_t *mac)
{
    if (nullptr != mac)
    {
        memcpy(m_apmac, mac, 6);
    }
}

int8_t WiFiInterface::scanNetworks(bool _async, bool _show_hidden, uint8_t _channel, uint8_t *ssid)
{
    return (int8_t)m_scancount;
}

void WiFiInterface::scanNetworksAsync(pdiutil::function<void(int)> _onComplete, bool _show_hidden)
{
    if (_onComplete)
    {
        _onComplete((int)m_scancount);
    }
}

pdiutil::string WiFiInterface::SSID(uint8_t _networkItem)
{
    if (_networkItem >= m_scancount)
    {
        return pdiutil::string();
    }
    return pdiutil::string(m_scanresults[_networkItem].m_ssid);
}

int32_t WiFiInterface::RSSI(uint8_t _networkItem)
{
    return (_networkItem < m_scancount) ? m_scanresults[_networkItem].m_rssi : 0;
}

uint8_t *WiFiInterface::BSSID(uint8_t _networkItem)
{
    return (_networkItem < m_scancount) ? m_scanresults[_networkItem].m_bssid : m_bssid;
}

bool WiFiInterface::get_bssid_within_scanned_nw_ignoring_connected_stations(char *ssid,
                                                                           uint8_t *bssid,
                                                                           uint8_t *ignorebssid,
                                                                           int _scanCount)
{
    if (nullptr == ssid || nullptr == bssid)
    {
        return false;
    }

    for (uint8_t i = 0; i < m_scancount && i < (uint8_t)_scanCount; i++)
    {
        if (0 != strcmp(m_scanresults[i].m_ssid, ssid))
        {
            continue;
        }

        if (nullptr != ignorebssid && 0 == memcmp(ignorebssid, m_scanresults[i].m_bssid, 6))
        {
            continue;
        }

        memcpy(bssid, m_scanresults[i].m_bssid, 6);
        return true;
    }

    return false;
}

bool WiFiInterface::getApsConnectedStations(pdiutil::vector<wifi_station_info_t> &stations)
{
    return m_apactive;
}

void WiFiInterface::enableNetworkStatusIndication()
{
}

void WiFiInterface::enableNAPT(bool enable)
{
}

void WiFiInterface::clearScanResults()
{
    m_scancount = 0;
}

bool WiFiInterface::addScanResult(const char *ssid, int32_t rssi, uint8_t channel)
{
    if (nullptr == ssid || m_scancount >= MOCKDEVICE_WIFI_MAX_SCAN_RESULTS)
    {
        return false;
    }

    mock_wifi_network_t &entry = m_scanresults[m_scancount];
    strncpy(entry.m_ssid, ssid, sizeof(entry.m_ssid) - 1);
    entry.m_ssid[sizeof(entry.m_ssid) - 1] = '\0';
    entry.m_rssi = rssi;
    entry.m_channel = channel;

    // a distinct bssid per entry so callers can tell them apart
    entry.m_bssid[0] = 0x0A;
    entry.m_bssid[1] = 0x0B;
    entry.m_bssid[2] = 0x0C;
    entry.m_bssid[3] = 0x0D;
    entry.m_bssid[4] = 0x0E;
    entry.m_bssid[5] = (uint8_t)(0x10 + m_scancount);

    m_scancount++;
    return true;
}

void WiFiInterface::setAssociationAllowed(bool allowed)
{
    m_associationallowed = allowed;
}

void WiFiInterface::dropConnection()
{
    m_connected = false;
}

void WiFiInterface::setStationIp(const ipaddress_t &ip)
{
    m_localip = ip;
}

uint32_t WiFiInterface::getBeginCount() const
{
    return m_begincount;
}

uint32_t WiFiInterface::getDisconnectCount() const
{
    return m_disconnectcount;
}

void WiFiInterface::clearCounters()
{
    m_begincount = 0;
    m_disconnectcount = 0;
}

WiFiInterface __i_wifi;
