/************************* Data Type Convertors *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "DataTypeConversions.h"

/**
 * This function convert bcd to uint8_t.
 *
 * @param   uint8_t val
 * @return  uint8_t
 */
uint8_t BcdToUint8(uint8_t val)
{
	return val - 6 * (val >> 4);
}

/**
 * This function convert uint8_t to bcd.
 *
 * @param   uint8_t val
 * @return  uint8_t
 */
uint8_t Uint8ToBcd(uint8_t val)
{
	return val + 6 * (val / 10);
}

/**
 * This function convert string to uint32_t.
 *
 * @param   const char* pString
 * @param   uint8_t _len|32
 * @return  uint32_t
 */
uint32_t StringToUint32(const char *pString, uint8_t _len)
{

	if (nullptr == pString)
	{
		return 0;
	}

	uint32_t value = 0;
	uint8_t n = 0;

	while ('0' == *pString || *pString == ' ' || *pString == '"' && n < _len)
	{
		pString++;
		n++;
	}

	while ('0' <= *pString && *pString <= '9' && n < _len)
	{
		value *= 10;
		value += *pString - '0';
		pString++;
		n++;
	}
	return value;
}

/**
 * This function convert string to uint16_t.
 *
 * @param   const char* pString
 * @param   uint8_t _len|32
 * @return  uint16_t
 */
uint16_t StringToUint16(const char *pString, uint8_t _len)
{

	if (nullptr == pString)
	{
		return 0;
	}

	uint16_t value = 0;
	uint8_t n = 0;

	while ('0' == *pString || *pString == ' ' || *pString == '"' && n < _len)
	{
		pString++;
		n++;
	}

	while ('0' <= *pString && *pString <= '9' && n < _len)
	{
		value *= 10;
		value += *pString - '0';
		pString++;
		n++;
	}
	return value;
}

/**
 * This function convert string to uint8_t.
 *
 * @param   const char* pString
 * @param   uint8_t _len|32
 * @return  uint8_t
 */
uint8_t StringToUint8(const char *pString, uint8_t _len)
{

	if (nullptr == pString)
	{
		return 0;
	}

	uint8_t value = 0, n = 0;

	while ('0' == *pString || *pString == ' ' || *pString == '"' && n < _len)
	{
		pString++;
		n++;
	}

	while ('0' <= *pString && *pString <= '9' && n < _len)
	{
		value *= 10;
		value += *pString - '0';
		pString++;
		n++;
	}
	return value;
}

/**
 * This function convert string to hex16.
 *
 * @param   const char* pString
 * @param   uint8_t _strlen
 * @return  uint16_t
 */
uint16_t StringToHex16(const char *pString, uint8_t _strlen)
{

	if (nullptr == pString)
	{
		return 0;
	}

	uint16_t value = 0;
	uint16_t hexValue = 0;
	uint16_t hexWeight = 1;

	for (uint8_t i = 0; i < _strlen - 1; i++)
		hexWeight *= 16;
	while (*pString == ' ' || *pString == '"')
		pString++;
	while (('0' <= *pString && *pString <= '9') ||
		   ('A' <= *pString && *pString <= 'F') ||
		   ('a' <= *pString && *pString <= 'f'))
	{
		if ('A' <= *pString && *pString <= 'F')
			value = 10 + (*pString - 'A');
		else if ('a' <= *pString && *pString <= 'f')
			value = 10 + (*pString - 'a');
		else
			value = (*pString - '0');
		hexValue += hexWeight * value;
		hexWeight /= 16;
		pString++;
	}
	return hexValue;
}
