/***************************** Custom EEPROM **********************************
This file is part of the Ewings Esp Stack. It is modified/edited copy of
arduino esp8266 eeprom library.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#include "Arduino.h"
#include "EW_EEPROM.h"

extern "C" {
#include "c_types.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "spi_flash.h"
}

extern "C" uint32_t _SPIFFS_end;

EW_EEPROMClass::EW_EEPROMClass(uint32_t sector)
: m_sector(sector),
  m_copy_sector(0),
  m_data(nullptr),
  m_size(0),
  m_dirty(false)
{
}

EW_EEPROMClass::EW_EEPROMClass(void)
: m_sector((((uint32_t)&_SPIFFS_end - 0x40200000) / SPI_FLASH_SEC_SIZE)),
  m_copy_sector(0),
  m_data(nullptr),
  m_size(0),
  m_dirty(false)
{
}

void EW_EEPROMClass::begin(size_t size) {
  if (size <= 0)
    return;
  if (size > SPI_FLASH_SEC_SIZE)
    size = SPI_FLASH_SEC_SIZE;

  size = (size + 3) & (~3);

  //In case begin() is called a 2nd+ time, don't reallocate if size is the same
  if( nullptr != m_data && size != m_size) {
    delete[] m_data;
    m_data = new uint8_t[size];
  } else if( nullptr == m_data ) {
    m_data = new uint8_t[size];
  }

  m_size = size;
  m_copy_sector = 255;

  noInterrupts();

  spi_flash_read(m_sector * SPI_FLASH_SEC_SIZE, reinterpret_cast<uint32_t*>(m_data), m_size);
  if( m_data[0] == EEPROM_VALIDITY_BYTES[0] && m_data[1] == EEPROM_VALIDITY_BYTES[1] &&
      m_data[2] == EEPROM_VALIDITY_BYTES[2] && m_data[3] == EEPROM_VALIDITY_BYTES[3] ){

  }else{
    spi_flash_read(m_copy_sector * SPI_FLASH_SEC_SIZE, reinterpret_cast<uint32_t*>(m_data), m_size);
    if( m_data[0] == EEPROM_VALIDITY_BYTES[0] && m_data[1] == EEPROM_VALIDITY_BYTES[1] &&
        m_data[2] == EEPROM_VALIDITY_BYTES[2] && m_data[3] == EEPROM_VALIDITY_BYTES[3] ){

        if(spi_flash_erase_sector(m_sector) == SPI_FLASH_RESULT_OK) {
          if(spi_flash_write(m_sector * SPI_FLASH_SEC_SIZE, reinterpret_cast<uint32_t*>(m_data), m_size) == SPI_FLASH_RESULT_OK) {
          }
        }
    }else{

        m_data[0] = EEPROM_VALIDITY_BYTES[0];
        m_data[1] = EEPROM_VALIDITY_BYTES[1];
        m_data[2] = EEPROM_VALIDITY_BYTES[2];
        m_data[3] = EEPROM_VALIDITY_BYTES[3];
    }
  }
  interrupts();

  m_dirty = false; //make sure dirty is cleared in case begin() is called 2nd+ time
}

void EW_EEPROMClass::end() {
  if (!m_size)
    return;

  commit();
  if( nullptr != m_data ) {
    delete[] m_data;
    m_data = nullptr;
  }
  m_size = 0;
  m_dirty = false;
}


uint8_t EW_EEPROMClass::read(int const address) {
  if ( address < 0 || (size_t)address >= m_size ){
    return 0;
  }
  if( nullptr == m_data ){
    return 0;
  }

  return m_data[address];
}

void EW_EEPROMClass::write(int const address, uint8_t const value) {
  if ( address < 0 || (size_t)address >= m_size ){
    return;
  }
  if( nullptr == m_data ){
    return;
  }

  // Optimise m_dirty. Only flagged if data written is different.
  uint8_t* pData = &m_data[address];
  if (*pData != value){
    *pData = value;
    m_dirty = true;
  }
}

bool EW_EEPROMClass::commit() {
  bool ret = false;
  if (!m_size){
    return false;
  }
  if(!m_dirty){
    return true;
  }
  if( nullptr == m_data ){
    return false;
  }

  noInterrupts();
  if(spi_flash_erase_sector(m_sector) == SPI_FLASH_RESULT_OK) {
    if(spi_flash_write(m_sector * SPI_FLASH_SEC_SIZE, reinterpret_cast<uint32_t*>(m_data), m_size) == SPI_FLASH_RESULT_OK) {
      m_dirty = false;
      ret = true;
    }
  }
  if(spi_flash_erase_sector(m_copy_sector) == SPI_FLASH_RESULT_OK) {
    if(spi_flash_write(m_copy_sector * SPI_FLASH_SEC_SIZE, reinterpret_cast<uint32_t*>(m_data), m_size) == SPI_FLASH_RESULT_OK) {
    }
  }

  interrupts();

  return ret;
}

uint8_t * EW_EEPROMClass::getDataPtr() {
  m_dirty = true;
  return m_data;
}

uint8_t const * EW_EEPROMClass::getConstDataPtr() const {
  return m_data;
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
EW_EEPROMClass EEPROM;
#endif
