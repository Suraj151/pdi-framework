/***************************** Custom EEPROM **********************************
This file is part of the Ewings Esp Stack. It is modified/edited copy of
arduino esp8266 eeprom library.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_EEPROM_H_
#define _EW_EEPROM_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define EEPROM_VALIDITY_BYTES "EWEE"

class EW_EEPROMClass
{
public:
  EW_EEPROMClass(uint32_t sector);
  EW_EEPROMClass(void);

  void begin(size_t size);
  uint8_t read(int const address);
  void write(int const address, uint8_t const val);
  bool commit();
  void end();

  uint8_t *getDataPtr();
  uint8_t const *getConstDataPtr() const;

  size_t length() { return m_size; }

  uint8_t &operator[](int const address) { return getDataPtr()[address]; }
  uint8_t const &operator[](int const address) const { return getConstDataPtr()[address]; }

private:
  uint32_t m_sector;
  uint32_t m_copy_sector;
  uint8_t *m_data;
  size_t m_size;
  bool m_dirty;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
extern EW_EEPROMClass EEPROM;
#endif

#endif
