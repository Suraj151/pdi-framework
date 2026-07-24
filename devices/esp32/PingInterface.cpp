/****************************** Ping Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "PingInterface.h"
#include "LoggerInterface.h"
#include <string.h>

volatile bool _host_resp = false;
static volatile bool _ping_complete = false;
static ping_stats_t _ping_stats = {0, 0, 0, 0, 0};
static uint32_t _ping_sum_ms = 0;
static CallBackVoidPointerArgFn _ping_pkt_cb = nullptr;
static esp_ping_handle_t _ping_hdl = nullptr;

// Invoked per received reply.
static void on_ping_success( esp_ping_handle_t hdl, void *args ){

  uint16_t seqno = 0;
  uint32_t elapsed = 0;
  esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
  esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed, sizeof(elapsed));

  _host_resp = true;
  _ping_stats.m_received++;
  if (_ping_stats.m_received == 1 || elapsed < _ping_stats.m_min_ms) _ping_stats.m_min_ms = elapsed;
  if (elapsed > _ping_stats.m_max_ms) _ping_stats.m_max_ms = elapsed;
  _ping_sum_ms += elapsed;
  _ping_stats.m_avg_ms = _ping_sum_ms / _ping_stats.m_received;

  if (_ping_pkt_cb) {
    ping_pkt_t pkt = { seqno, true, elapsed };
    _ping_pkt_cb((void*)&pkt);
  }
}

// Invoked per timed-out request.
static void on_ping_timeout( esp_ping_handle_t hdl, void *args ){

  uint16_t seqno = 0;
  esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));

  _host_resp = false;
  if (_ping_pkt_cb) {
    ping_pkt_t pkt = { seqno, false, 0 };
    _ping_pkt_cb((void*)&pkt);
  }
}

// Invoked once after the whole session finishes.
static void on_ping_end( esp_ping_handle_t hdl, void *args ){

  uint32_t transmitted = 0;
  esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
  _ping_stats.m_transmitted = (uint16_t)transmitted;
  _ping_complete = true;
}

/**
 * PingInterface constructor.
 */
PingInterface::PingInterface():
  m_wifi(nullptr)
{
}

/**
 * PingInterface destructor
 */
PingInterface::~PingInterface(){
  this->m_wifi = nullptr;
}

void PingInterface::init_ping( iWiFiInterface* _wifi ){

  this->m_wifi = _wifi;
}

/**
 * Ping the host
 */
bool PingInterface::ping( const ipaddress_t &target, uint16_t count, CallBackVoidPointerArgFn on_packet ){

  ipaddress_t _target = target;
  if( !_target.isSet() ) return false;

  // tear down any previous session
  if( nullptr != _ping_hdl ){
    esp_ping_stop(_ping_hdl);
    esp_ping_delete_session(_ping_hdl);
    _ping_hdl = nullptr;
  }

  memset(&_ping_stats, 0, sizeof(_ping_stats));
  _ping_sum_ms = 0;
  _ping_complete = false;
  _ping_pkt_cb = on_packet;

  esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
  config.count = count;
  config.interval_ms = PING_COARSE;
  config.timeout_ms = PING_TIMEOUT_MS;
  IP_ADDR4(&config.target_addr, _target[0], _target[1], _target[2], _target[3]);

  esp_ping_callbacks_t cbs;
  cbs.cb_args = nullptr;
  cbs.on_ping_success = on_ping_success;
  cbs.on_ping_timeout = on_ping_timeout;
  cbs.on_ping_end = on_ping_end;

  pdiutil::string ipstr = _target;
  LogFmtI("\nPing ip: %s\n", ipstr.c_str());

  if( esp_ping_new_session(&config, &cbs, &_ping_hdl) != ESP_OK ){
    _ping_hdl = nullptr;
    return false;
  }
  return (esp_ping_start(_ping_hdl) == ESP_OK);
}

bool PingInterface::isPingComplete(){

  return _ping_complete;
}

bool PingInterface::isHostRespondingToPing(){

  return _host_resp;
}

const ping_stats_t &PingInterface::getPingStats(){

  return _ping_stats;
}

PingInterface __i_ping;
