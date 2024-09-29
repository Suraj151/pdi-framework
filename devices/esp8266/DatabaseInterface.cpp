/***************************** Database Interface *****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "DatabaseInterface.h"

/**
 * begin eeprom configs.
 *
 * @param uint16_t  _size
 */
void DatabaseInterface::beginConfigs(uint32_t _size)
{
  EEPROM.begin(_size);
}

/**
 * clear eeprom by writing zero ot its all locations.
 */
void DatabaseInterface::cleanAllConfigs(void)
{
  for (int i = 0; i < DATABASE_MAX_SIZE; i++)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.end();
}

/**
 * check whether database configs are valid
 *
 * @return bool
 */
bool DatabaseInterface::isValidConfigs(void)
{
  return (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
          EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
          EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2]);
}

/**
 * maximum database size can be stored
 *
 * @return max db size
 */
uint32_t DatabaseInterface::getMaxDBSize()
{
    return DATABASE_MAX_SIZE;
}

DatabaseInterface __i_db;
