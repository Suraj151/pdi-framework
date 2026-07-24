/****************************** Ping Interface *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _PING_INTERFACE_H_
#define _PING_INTERFACE_H_

#include "esp32.h"
#include <interface/pdi/middlewares/iPingInterface.h>

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000
#endif

/** ping delay - in milliseconds */
#ifndef PING_COARSE
#define PING_COARSE     500
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

#define DEFAULT_PING_MAX_COUNT 1
#define PING_TIMEOUT_MS 1000

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
    bool ping( const ipaddress_t &target, uint16_t count = 1,
               CallBackVoidPointerArgFn on_packet = nullptr ) override;
    bool isPingComplete( void ) override;
    bool isHostRespondingToPing( void ) override;
    const ping_stats_t &getPingStats( void ) override;

  protected:
    /**
		 * @var	iWiFiInterface*|&WiFi wifi
		 */
    iWiFiInterface  *m_wifi;
};

#endif
