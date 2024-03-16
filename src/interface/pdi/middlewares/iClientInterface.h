/***************************** Client Interface ******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_CLIENT_INTERFACE_H_
#define _I_CLIENT_INTERFACE_H_

#include <interface/interface_includes.h>

/**
 * iClientInterface class
 */
class iClientInterface
{

public:
  /**
   * iClientInterface constructor.
   */
  iClientInterface() {}
  /**
   * iClientInterface destructor.
   */
  virtual ~iClientInterface() {}

  // connect/disconnect api
  virtual int16_t connect(const uint8_t *host, uint16_t port) = 0;
  virtual int16_t disconnect() = 0;

  // data sending api
  virtual uint32_t write(uint8_t c) = 0;
  virtual uint32_t write(const uint8_t *c_str) = 0;
  virtual uint32_t write(const uint8_t *c_str, uint32_t size) = 0;

  // received data read api
  virtual uint8_t read() = 0;
  virtual uint32_t read(uint8_t *buf, uint32_t size) = 0;

  // useful api
  virtual int32_t available() = 0;
  virtual int8_t connected() = 0;
  virtual void setTimeout(uint32_t timeout) = 0;
  virtual void flush() = 0;
  virtual iClientInterface* getNewInstance() = 0;
  static void releaseInstance( iClientInterface **instance )
  {
    if( nullptr != *instance )
    {
      delete *instance;
      *instance = nullptr;
    }
  }
};

#endif
