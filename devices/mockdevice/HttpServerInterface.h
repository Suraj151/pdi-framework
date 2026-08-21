/************************* Http Server Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _MOCKDEVICE_HTTP_SERVER_INTERFACE_H_
#define _MOCKDEVICE_HTTP_SERVER_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/impl/middlewares/HttpServerInterfaceImpl.h>

/**
 * HttpServerInterface class
 */
class HttpServerInterface : public HttpServerInterfaceImpl
{

public:
  /**
   * HttpServerInterface constructor.
   */
  HttpServerInterface();

  /**
   * HttpServerInterface destructor.
   */
  ~HttpServerInterface();
};

#endif // _MOCKDEVICE_HTTP_SERVER_INTERFACE_H_
