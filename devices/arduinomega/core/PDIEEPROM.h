/***************************** Custom EEPROM **********************************
This file is part of the pdi stack. It is modified/edited copy of
arduino esp8266 eeprom library.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _PDI_EEPROM_H_
#define _PDI_EEPROM_H_

#include <EEPROM.h>

#define EEPROM_VALIDITY_BYTES "EWEE"

class PDIEEPROMClass : public EEPROMClass
{
public:
  /**
   * PDIEEPROMClass constructor.
   */
  PDIEEPROMClass() {}
  /**
   * PDIEEPROMClass destructor.
   */
  virtual ~PDIEEPROMClass() {}

};

extern PDIEEPROMClass PDIEEPROM;

#endif
