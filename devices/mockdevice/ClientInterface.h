/****************************** Client Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _MOCKDEVICE_CLIENT_INTERFACE_H_
#define _MOCKDEVICE_CLIENT_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/modules/wifi/iWiFiClientInterface.h>

/**
 * ClientInterface class
 */
class ClientInterface : public iWiFiClientInterface
{

public:
  /**
   * ClientInterface constructor.
   */
  ClientInterface(){}
  /**
   * ClientInterface destructor.
   */
  ~ClientInterface(){}

  // connect/disconnect api
  int16_t connect(const uint8_t *host, uint16_t port) override{return 0;}
  int16_t disconnect() override{return 0;}

  // data sending api
  uint32_t write(uint8_t c) override{return 0;}
  uint32_t write(const uint8_t *c_str) override{return 0;}
  uint32_t write(const uint8_t *c_str, uint32_t size) override{return 0;}

  // received data read api
  uint8_t read() override{return 0;}
  uint32_t read(uint8_t *buf, uint32_t size) override{return 0;}

  // useful api
  int32_t available() override{return 0;}
  int8_t connected() override{return 0;}
  void setTimeout(uint32_t timeout) override{}
  void flush() override{}
  iClientInterface* getNewInstance() override{return nullptr;}

};

#endif
