/************************ i WiFi Server Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _I_WIFI_SERVER_INTERFACE_H_
#define _I_WIFI_SERVER_INTERFACE_H_

#include <config/Common.h>
#include <interface/interface_includes.h>
#include <interface/pdi/middlewares/iServerInterface.h>

// forward declaration of derived class for this interface
class WiFiServerInterface;

/**
 * iWiFiServerInterface class
 */
class iWiFiServerInterface : public iServerInterface
{

public:
  /**
   * iWiFiServerInterface constructor.
   */
  iWiFiServerInterface() {}
  /**
   * iWiFiServerInterface destructor.
   */
  virtual ~iWiFiServerInterface() {}
};

// derived class must define this
extern WiFiServerInterface __i_wifi_server;

#endif
