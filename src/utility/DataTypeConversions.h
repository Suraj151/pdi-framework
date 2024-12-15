/************************* Data Type Convertors *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef __DATATYPECONVERSIONS_H__
#define __DATATYPECONVERSIONS_H__

#include "DataTypeDef.h"

uint8_t BcdToUint8(uint8_t val);
uint8_t Uint8ToBcd(uint8_t val);
uint32_t StringToUint32(const char *pString, uint8_t _len = 32);
uint16_t StringToUint16(const char *pString, uint8_t _len = 32);
uint8_t StringToUint8(const char *pString, uint8_t _len = 32);
uint16_t StringToHex16(const char *pString, uint8_t _strlen);
void Int32ToString(int32_t val, char *pString, uint8_t _maxlen, uint8_t _padmax=0);
void Int64ToString(int64_t val, char *pString, uint8_t _maxlen, uint8_t _padmax=0);
uint8_t Int32DigitCount(int32_t x);
uint8_t Int64DigitCount(int64_t x);

#endif
