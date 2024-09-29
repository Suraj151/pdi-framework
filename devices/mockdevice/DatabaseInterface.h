/***************************** Database Interface *****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _MOCKDEVICE_DATABASE_INTERFACE_H_
#define _MOCKDEVICE_DATABASE_INTERFACE_H_

#include "mockdevice.h"
#include <interface/pdi/iDatabaseInterface.h>

/**
 * define eeprom max size available here
 */
// #define DATABASE_MAX_SIZE SPI_FLASH_SEC_SIZE

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

  void beginConfigs(uint32_t _size) override{}
  void cleanAllConfigs() override{}
  bool isValidConfigs() override{return 0;}
  uint32_t getMaxDBSize() override{return 0;}

  template <typename T>
  void saveConfig(uint16_t _address, T *_object){}

  template <typename T>
  void loadConfig(uint16_t _address, T *_object){}

  template <typename T>
  void clearConfig(uint16_t _address){}
};

#endif
