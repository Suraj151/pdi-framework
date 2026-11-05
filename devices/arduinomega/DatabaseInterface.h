/***************************** Database Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _ARDUINOMEGA_DATABASE_INTERFACE_H_
#define _ARDUINOMEGA_DATABASE_INTERFACE_H_

#include "arduinomega.h"
#include <interface/pdi/iDatabaseInterface.h>
#include "core/PDIEEPROM.h"

/**
 * define eeprom max size available here
 */
#define DATABASE_MAX_SIZE 4096

/**
 * DatabaseInterface class
 */
class DatabaseInterface : public iDatabaseInterface
{

public:
  /**
   * DatabaseInterface constructor.
   */
  DatabaseInterface() {}
  /**
   * DatabaseInterface destructor.
   */
  virtual ~DatabaseInterface() {}

  void beginConfigs(uint32_t _size) override;
  void endConfigs() override;
  void cleanAllConfigs() override;
  uint32_t getMaxDBSize() override;

  uint8_t readByte(uint32_t _address) override;
  void writeByte(uint32_t _address, uint8_t _value) override;
  void commitConfigs() override;
};


#endif
