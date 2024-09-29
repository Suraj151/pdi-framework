/****************************** Ping Interface *******************************
This file is part of the Ewings Esp Stack.

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
    void init_ping( iWiFiInterface* _wifi ) override{}
    bool ping( void ) override{return 0;}
    bool isHostRespondingToPing( void ) override{return 0;}

  protected:
    /**
		 * @var	iWiFiInterface*|&WiFi wifi
		 */
    iWiFiInterface  *m_wifi;
};

#endif
