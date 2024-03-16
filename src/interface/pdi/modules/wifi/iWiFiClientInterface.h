/************************ i WiFi Client Interface ******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _I_WIFI_CLIENT_INTERFACE_H_
#define _I_WIFI_CLIENT_INTERFACE_H_

#include <interface/interface_includes.h>
#include <interface/pdi/middlewares/iClientInterface.h>

// forward declaration of derived class for this interface
class WiFiClientInterface;

/**
 * iWiFiClientInterface class
 */
class iWiFiClientInterface : public iClientInterface
{

public:
  /**
   * iWiFiClientInterface constructor.
   */
  iWiFiClientInterface() {}
  /**
   * iWiFiClientInterface destructor.
   */
  virtual ~iWiFiClientInterface() {}
};

// derived class must define this
extern WiFiClientInterface __i_wifi_client;

#endif
