/****************************** Ping Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _PING_INTERFACE_H_
#define _PING_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/middlewares/iPingInterface.h>

// This function is called when a ping is received or the request times out:
static void ICACHE_FLASH_ATTR ping_recv_cb(void* arg, void* pdata);
// This function is called when a ping is sent
static void ICACHE_FLASH_ATTR ping_sent_cb(void* arg, void* pdata);

/**
 * PingInterface class
 */
class PingInterface : public iPingInterface {

  public:

    /**
     * PingInterface constructor.
     */
    PingInterface();
    /**
		 * PingInterface destructor
		 */
    ~PingInterface();

    /**
     * initialize ping
     */
    void init_ping( iWiFiInterface* _wifi ) override;
    bool ping( void ) override;
    bool isHostRespondingToPing( void ) override;

    ping_option m_opt;
    // bool host_resp;

  protected:
    /**
		 * @var	iWiFiInterface*|&WiFi wifi
		 */
    iWiFiInterface  *m_wifi;
};

#endif
