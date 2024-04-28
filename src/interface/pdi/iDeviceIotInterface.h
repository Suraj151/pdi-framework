/************************** Device Iot Interface *******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _I_DEVICE_IOT_INTERFACE_H_
#define _I_DEVICE_IOT_INTERFACE_H_

#include <interface/interface_includes.h>

/**
 * iDeviceIotInterface class
 */
class iDeviceIotInterface {

  public:

    /**
     * iDeviceIotInterface constructor.
     */
    iDeviceIotInterface(){}
    /**
     * iDeviceIotInterface destructor.
     */
    virtual ~iDeviceIotInterface(){}

    virtual void init( void ) = 0;
    virtual void sampleHook( void ) = 0;
    virtual void dataHook( std::string &_payload ) = 0;
    virtual void resetSampleHook( void ) = 0;
};

#endif
