/*************************** EEPROM Database **********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EEPROM_DATABASE_H_
#define _EEPROM_DATABASE_H_

/**
 * define eeprom max size available here
 */
#define DATABASE_MAX_SIZE      SPI_FLASH_SEC_SIZE

#include <database/eeprom/EW_EEPROM.h>
#include <utility/Utility.h>
#include <config/Config.h>

void beginConfigs(  uint16_t _size );
void cleanAllConfigs( void );
bool isValidConfigs( void );

/**
 * template to save table in database by their address from table object
 *
 * @param   type of database table struct  _object
 * @param   uint16_t  	configStart
 */
template <typename T> void saveConfig ( T * _object, uint16_t configStart ) {
  bool _data_written = false;
  for (unsigned int i = 0; i < sizeof((*_object)); i++) {
    if ((char)EEPROM.read(configStart + i) != *((char*) & (*_object) + i)) {
      _data_written = true;
      EEPROM.write(configStart + i, *((char*) & (*_object) + i));
    }
  }
  if( _data_written ){
    EEPROM.commit();
  }
}

/**
 * template to clear tables in database by their address
 *
 * @param   const type of database table struct  _object
 * @param   uint16_t  	_address
 */
template <typename T> void clearConfigs( uint16_t _address ) {
  T _t;
  // PROGMEM_readAnything ( _object, _t );
  saveConfig( &_t, _address );
  _ClearObject( &_t );
}

/**
 * template to load table from database by their address in table object
 *
 * @param   type of database table struct  _object
 * @param   uint16_t  	configStart
 */
template <typename T> void loadConfig ( T * _object, uint16_t configStart ) {
  if ( isValidConfigs() ){
    for (unsigned int i = 0; i < sizeof((*_object)); i++){
      *((char*) & (*_object) + i) = EEPROM.read(configStart + i);
    }
  }
}

#endif
