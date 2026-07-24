/****************************** Ping Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MOCKDEVICE_PING_INTERFACE_H_
#define _MOCKDEVICE_PING_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/middlewares/iPingInterface.h>
#include "LoggerInterface.h"

/**
 * PingInterface class
 */
class PingInterface : public iPingInterface {

  public:

    /**
     * PingInterface constructor.
     */
    PingInterface(){}
    /**
		 * PingInterface destructor
		 */
    ~PingInterface(){}

    /**
     * initialize ping
     */
    void init_ping( iWiFiInterface* _wifi ) override{ m_wifi = _wifi; }
    bool ping( const ipaddress_t &target, uint16_t count = 1,
               CallBackVoidPointerArgFn on_packet = nullptr ) override{
      ipaddress_t _target = target;
      if( !_target.isSet() ) return false;
      m_stats = ping_stats_t{ count, 0, 0, 0, 0 };
      if( on_packet ){
        for( uint16_t i = 1; i <= count; i++ ){
          ping_pkt_t pkt = { i, false, 0 };
          on_packet((void*)&pkt);
        }
      }
      return true;
    }
    bool isPingComplete( void ) override{ return true; }
    bool isHostRespondingToPing( void ) override{ return false; }
    const ping_stats_t &getPingStats( void ) override{ return m_stats; }

  protected:
    /**
		 * @var	iWiFiInterface*|&WiFi wifi
		 */
    iWiFiInterface  *m_wifi;
    ping_stats_t     m_stats = {0, 0, 0, 0, 0};
};

#endif
