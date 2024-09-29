/***************************** Database Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _I_DATABASE_INTERFACE_H_
#define _I_DATABASE_INTERFACE_H_

#include <interface/interface_includes.h>


// forward declaration of derived class for this interface
class DatabaseInterface;

/**
 * iDatabaseInterface class
 */
class iDatabaseInterface
{

public:
  /**
   * iDatabaseInterface constructor.
   */
  iDatabaseInterface() {}
  /**
   * iDatabaseInterface destructor.
   */
  virtual ~iDatabaseInterface() {}

  virtual void beginConfigs(uint32_t _size) = 0;
  virtual void cleanAllConfigs() = 0;
  virtual bool isValidConfigs() = 0;
  virtual uint32_t getMaxDBSize() = 0;

  /**
   * Below template methods are must to define by derived
   */

  // template <typename T>
  // void saveConfig(uint16_t _address, T *_object);

  // template <typename T>
  // void loadConfig(uint16_t _address, T *_object);

  // template <typename T>
  // void clearConfig(uint16_t _address);
};

// derived class must define this
extern DatabaseInterface __i_db;

#endif
