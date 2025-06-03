/****************************** Ping Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "PingInterface.h"
#include "LoggerInterface.h"

volatile bool _host_resp = false;
uint32_t _pinghostip = 0x08080808;
static const char _pinghostname[] = "google.com";
// IPAddress PING_TARGET(8,8,8,8);

// This function is called when a ping is received or the request times out:
void pingResults( ping_target_id_t found_id, esp_ping_found * pf ) {

  if( nullptr != pf ){

    LogFmtI("\nPing Result: AvgTime(%f), Sent(%d), Recv(%d), Err(%d), minms(%d), maxms(%d)\n", 
      ((float)pf->total_time / (float)pf->recv_count), 
      pf->send_count,
      pf->recv_count,
      pf->err_count,
      pf->min_time,
      pf->max_time
    );

    _host_resp = ( pf->send_count == pf->recv_count );
  }
  // ping_deinit();
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

bool PingInterface::ping(){

  ipaddress_t _ip;
  this->m_wifi->hostByName(_pinghostname, _ip, 0); // third param will be unused in esp32

  uint32_t ping_count = DEFAULT_PING_MAX_COUNT;  //how many pings per report
  uint32_t ping_timeout = PING_TIMEOUT_MS; //mS till we consider it timed out
  uint32_t ping_delay = PING_COARSE; //mS between pings
  uint32_t ping_target = _ip;
  void* respFunction = reinterpret_cast<void*>(pingResults);

  //return _ip!=INADDR_NONE ? ping_start(&this->_opt) : false;
  esp_ping_set_target(PING_TARGET_IP_ADDRESS_COUNT, &ping_count, sizeof(uint32_t));
  esp_ping_set_target(PING_TARGET_RCV_TIMEO, &ping_timeout, sizeof(uint32_t));
  esp_ping_set_target(PING_TARGET_DELAY_TIME, &ping_delay, sizeof(uint32_t));
  esp_ping_set_target(PING_TARGET_IP_ADDRESS, &_pinghostip, sizeof(uint32_t));
  esp_ping_set_target(PING_TARGET_RES_FN, respFunction, sizeof(respFunction));

  int res = ping_init();
  LogFmtI("\nPinging ip: %s : %d\n", ((pdiutil::string)_ip).c_str(), res);

  return (0 == res);
}

bool PingInterface::isHostRespondingToPing(){

  return _host_resp;
}

PingInterface __i_ping;
