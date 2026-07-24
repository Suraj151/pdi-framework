/****************************** Ping Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "PingInterface.h"
#include "LoggerInterface.h"
#include "SerialInterface.h"
#ifdef ENABLE_CONTEXTUAL_EXECUTION
#include "threading/Preemptive.h"
#endif

volatile bool _host_resp = false;
static volatile bool _ping_complete = false;
static ping_stats_t _ping_stats = {0, 0, 0, 0, 0};
static uint32_t _ping_sum_ms = 0;
static uint16_t _ping_seq = 0;
static uint16_t _ping_count = 0;
static CallBackVoidPointerArgFn _ping_pkt_cb = nullptr;

// This function is called when a ping is received or the request times out.
static void ICACHE_FLASH_ATTR ping_recv_cb (void* arg, void *pdata){

  struct ping_resp *pingrsp = (struct ping_resp *)pdata;

  // recv fires exactly once per ping (reply OR timeout), so the packet
  // counter is the reliable completion signal — unlike the SDK's sent
  // callback, which fires before a final packet's timeout.
  _ping_seq++;
  _ping_stats.m_transmitted = _ping_seq;
  _host_resp = (pingrsp->bytes > 0);
  if (pingrsp->bytes > 0) {
    _ping_stats.m_received++;
    uint32_t rtt = pingrsp->resp_time;
    if (_ping_stats.m_received == 1 || rtt < _ping_stats.m_min_ms) _ping_stats.m_min_ms = rtt;
    if (rtt > _ping_stats.m_max_ms) _ping_stats.m_max_ms = rtt;
    _ping_sum_ms += rtt;
    _ping_stats.m_avg_ms = _ping_sum_ms / _ping_stats.m_received;
  }
  if (_ping_seq >= _ping_count) _ping_complete = true;

  // deliver each packet to the caller (e.g. the `ping` command prints it live);
  // fall through to the legacy serial log only when no callback is set.
  if (_ping_pkt_cb) {
    ping_pkt_t pkt = { _ping_seq, (pingrsp->bytes > 0), pingrsp->resp_time };
    _ping_pkt_cb((void*)&pkt);
    return;
  }

#ifdef ENABLE_CONTEXTUAL_EXECUTION
  if (
    __i_preemptive_scheduler.is_task_context() || 
    !__i_preemptive_scheduler.is_sched_active()
  ) {
    if (_host_resp) {
      LogFmtI("\nPing: Reply bytes=%d time=%dms\n", pingrsp->bytes, pingrsp->resp_time);
    } else {
      LogI("\nPing: Request timed out\n");
    }
  } else if (__serial_uart.m_mutex.try_lock()) {
    if (_host_resp) {
      LogFmtI("\nPing: Reply bytes=%d time=%dms\n", pingrsp->bytes, pingrsp->resp_time);
    } else {
      LogI("\nPing: Request timed out\n");
    }
    __serial_uart.m_mutex.unlock();
  }
  // else: serial mutex held by another task — skip this log to avoid deadlock.
#else
  if (_host_resp) {
    LogFmtI("\nPing: Reply bytes=%d time=%dms\n", pingrsp->bytes, pingrsp->resp_time);
  } else {
    LogI("\nPing: Request timed out\n");
  }
#endif
}

// End-of-run callback. Fires ~coarse_time after the last send, which can be
// before that packet's timeout — so completion is tracked in the recv
// callback (per-packet count) instead of here.
static void ICACHE_FLASH_ATTR ping_sent_cb (void* arg, void *pdata){
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
  memset(&this->m_opt, 0, sizeof(struct ping_option));
  this->m_opt.coarse_time = 0;
  ping_regist_sent(&this->m_opt, ping_sent_cb);
  ping_regist_recv(&this->m_opt, ping_recv_cb);
}

bool PingInterface::ping( const ipaddress_t &target, uint16_t count, CallBackVoidPointerArgFn on_packet ){

  ipaddress_t _target = target;
  if (!_target.isSet()) return false;

  memset(&_ping_stats, 0, sizeof(_ping_stats));
  _ping_sum_ms = 0;
  _ping_seq = 0;
  _ping_count = count;
  _ping_complete = false;
  _ping_pkt_cb = on_packet;

  this->m_opt.count = count;
  this->m_opt.ip = (uint32_t)_target;
  this->m_opt.coarse_time = 0;

  pdiutil::string ipstr = _target;
  LogFmtI("\nPing ip: %s\n", ipstr.c_str());

  return ping_start(&this->m_opt) ? true : false;
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
