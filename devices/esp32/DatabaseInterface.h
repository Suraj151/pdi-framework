/***************************** Database Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _DATABASE_INTERFACE_H_
#define _DATABASE_INTERFACE_H_

#include "esp32.h"
#include <interface/pdi/iDatabaseInterface.h>
#include "core/EEPROM.h"

/**
 * define eeprom max size available here
 */
#define DATABASE_MAX_SIZE SPI_FLASH_SEC_SIZE

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
  void cleanAllConfigs() override;
  bool isValidConfigs() override;
  uint32_t getMaxDBSize() override;

  /**
   * template to save table in database by their address from table object
   *
   * @param   uint16_t  	_address
   * @param   type of database table struct  _object
   */
  template <typename T>
  void saveConfig(uint16_t _address, T *_object)
  {
    bool _data_written = false;
    for (unsigned int i = 0; i < sizeof((*_object)); i++)
    {
      if ((char)EEPROM.read(_address + i) != *((char *)&(*_object) + i))
      {
        _data_written = true;
        EEPROM.write(_address + i, *((char *)&(*_object) + i));
      }
    }
    if (_data_written)
    {
      EEPROM.commit();
    }
  }

  /**
   * template to load table from database by their address in table object
   *
   * @param   uint16_t  	_address
   * @param   type of database table struct  _object
   */
  template <typename T>
  void loadConfig(uint16_t _address, T *_object)
  {
    if (isValidConfigs())
    {
      for (unsigned int i = 0; i < sizeof((*_object)); i++)
      {
        *((char *)&(*_object) + i) = EEPROM.read(_address + i);
      }
    }
  }

  /**
   * template to clear tables in database by their address
   *
   * @param   uint16_t  	_address
   */
  template <typename T>
  void clearConfig(uint16_t _address)
  {
    T _t;
    saveConfig(_address, &_t);
    _ClearObject(&_t);
  }
};


#endif
