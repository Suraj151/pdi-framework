/****************************** ping service **********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "PingServiceProvider.h"

volatile bool _host_resp = false;
const uint32_t _pinghostip = 0x08080808;
static const char _pinghostname[] = "google.com";
// IPAddress PING_TARGET(8,8,8,8);

// This function is called when a ping is received or the request times out:
static void ICACHE_FLASH_ATTR ping_recv_cb (void* arg, void *pdata){

  struct ping_resp *pingrsp = (struct ping_resp *)pdata;

  if (pingrsp->bytes > 0) {
    LogFmtI("\nPing: Reply bytes=%d time=%dms\n", pingrsp->bytes, pingrsp->resp_time);
    _host_resp = true;
  } else {
    LogI("\nPing: Request timed out\n");
    _host_resp = false;
  }
}

/**
 * PingServiceProvider constructor.
 */
PingServiceProvider::PingServiceProvider():
  m_wifi(nullptr)
{
}

/**
 * PingServiceProvider destructor
 */
PingServiceProvider::~PingServiceProvider(){
  this->m_wifi = nullptr;
}

void PingServiceProvider::init_ping( iWiFiInterface* _wifi ){

  this->m_wifi = _wifi;
  memset(&this->m_opt, 0, sizeof(struct ping_option));
  this->m_opt.count = 1;
  this->m_opt.ip = _pinghostip;
  this->m_opt.coarse_time = 0;
  // m_opt.sent_function = NULL;
  // m_opt.recv_function = NULL;
  // m_opt.reverse = NULL;
  ping_regist_sent(&this->m_opt, NULL);
  // ping_regist_recv(&this->m_opt, reinterpret_cast<ping_recv_function>(&PingServiceProvider::ping_recv));
  ping_regist_recv(&this->m_opt, ping_recv_cb);
}

bool PingServiceProvider::ping(){

  IPAddress _ip(this->m_opt.ip);

  if( nullptr != this->m_wifi ){
    this->m_wifi->hostByName(_pinghostname, _ip, 1500);
    this->m_opt.ip = (uint32_t)_ip;
  }
  LogFmtI("\nPing ip: %s\n", _ip.toString().c_str());
  return _ip.isSet() ? ping_start(&this->m_opt) : false;
}

bool PingServiceProvider::isHostRespondingToPing(){

  return _host_resp;
}

PingServiceProvider __ping_service;
