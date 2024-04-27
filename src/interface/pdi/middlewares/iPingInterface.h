/***************************** Ping Interface *********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _I_PING_INTERFACE_H_
#define _I_PING_INTERFACE_H_

#include <interface/pdi/modules/wifi/iWiFiInterface.h>


// forward declaration of derived class for this interface
class PingInterface;

/**
 * iPingInterface class
 */
class iPingInterface {

  public:

    /**
     * iPingInterface constructor.
     */
    iPingInterface(){}
    /**
		 * iPingInterface destructor
		 */
    virtual ~iPingInterface(){}

    /**
     * initialize ping
     */
    virtual void init_ping( iWiFiInterface* _wifi ) = 0;
    virtual bool ping( void ) = 0;
    virtual bool isHostRespondingToPing( void ) = 0;
};

extern PingInterface __i_ping;

#endif
