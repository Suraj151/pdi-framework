/***************************** Database Interface *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include "DatabaseInterface.h"
#include <stdio.h>
#include <string.h>

/**
 * DatabaseInterface constructor.
 */
DatabaseInterface::DatabaseInterface() : m_size(DATABASE_MAX_SIZE)
{
  memset(m_store, 0, DATABASE_MAX_SIZE);
}

/**
 * DatabaseInterface destructor.
 */
DatabaseInterface::~DatabaseInterface()
{
}

uint8_t DatabaseInterface::readByte(uint32_t address) const
{
  return address < DATABASE_MAX_SIZE ? m_store[address] : 0;
}

void DatabaseInterface::writeByte(uint32_t address, uint8_t value)
{
  if (address < DATABASE_MAX_SIZE)
  {
    m_store[address] = value;
  }
}

/**
 * push the store to its backing file when one is attached
 */
void DatabaseInterface::commit()
{
  if (m_backingfile.size() == 0)
  {
    return;
  }

  FILE *f = fopen(m_backingfile.c_str(), "wb");
  if (nullptr == f)
  {
    return;
  }

  fwrite(m_store, 1, DATABASE_MAX_SIZE, f);
  fclose(f);
}

/**
 * begin configs.
 *
 * @param uint32_t  _size
 */
void DatabaseInterface::beginConfigs(uint32_t _size)
{
  m_size = _size > DATABASE_MAX_SIZE ? DATABASE_MAX_SIZE : _size;
}

/**
 * clear store by writing zero to all of its locations.
 */
void DatabaseInterface::cleanAllConfigs(void)
{
  memset(m_store, 0, DATABASE_MAX_SIZE);
  commit();
}

/**
 * check whether database configs are valid
 *
 * @return bool
 */
bool DatabaseInterface::isValidConfigs(void)
{
  return (readByte(CONFIG_START + 0) == CONFIG_VERSION[0] &&
          readByte(CONFIG_START + 1) == CONFIG_VERSION[1] &&
          readByte(CONFIG_START + 2) == CONFIG_VERSION[2]);
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

bool DatabaseInterface::attachBackingFile(const char *path)
{
  if (nullptr == path)
  {
    return false;
  }

  m_backingfile = path;

  FILE *f = fopen(path, "rb");
  if (nullptr != f)
  {
    fread(m_store, 1, DATABASE_MAX_SIZE, f);
    fclose(f);
    return true;
  }

  commit();
  return true;
}

void DatabaseInterface::detachBackingFile()
{
  m_backingfile.clear();
}

DatabaseInterface __i_db;
