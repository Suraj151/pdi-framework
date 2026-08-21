/***************************** Database Interface *****************************
This file is part of the pdi stack.

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
#define DATABASE_MAX_SIZE 4096

/**
 * DatabaseInterface class
 *
 * Keeps the config store in a plain byte array. A backing file can be attached
 * so a store survives across runs the way real NVM does.
 */
class DatabaseInterface : public iDatabaseInterface
{

public:
  /**
   * DatabaseInterface constructor.
   */
  DatabaseInterface();

  /**
   * DatabaseInterface destructor.
   */
  virtual ~DatabaseInterface();

  void beginConfigs(uint32_t _size) override;
  void cleanAllConfigs() override;
  bool isValidConfigs() override;
  uint32_t getMaxDBSize() override;

  /**
   * @brief Back the store with a file, loading it when it already exists.
   *        Every later write is persisted to it.
   */
  bool attachBackingFile(const char *path);

  /**
   * @brief Drop the backing file and keep the store in memory only.
   */
  void detachBackingFile();

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
    for (size_t i = 0; i < sizeof((*_object)); i++)
    {
      if ((char)readByte(_address + i) != *((char *)&(*_object) + i))
      {
        _data_written = true;
        writeByte(_address + i, *((char *)&(*_object) + i));
      }
    }
    if (_data_written)
    {
      commit();
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
      for (size_t i = 0; i < sizeof((*_object)); i++)
      {
        *((char *)&(*_object) + i) = readByte(_address + i);
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

private:
  uint8_t m_store[DATABASE_MAX_SIZE];
  uint32_t m_size;
  pdiutil::string m_backingfile;

  uint8_t readByte(uint32_t address) const;
  void writeByte(uint32_t address, uint8_t value);
  void commit();
};

#endif // _MOCKDEVICE_DATABASE_INTERFACE_H_
