/************************* Data Type Converters *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The DataTypeConversions utility provides a collection of helper functions for
converting between various data types, such as binary-coded decimal (BCD),
strings, integers, and hexadecimal values. These functions are designed to
simplify data manipulation and formatting tasks within the PDI stack.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef __DATA_TYPE_CONVERSIONS_H__
#define __DATA_TYPE_CONVERSIONS_H__

#include "DataTypeDef.h"

/**
 * @brief Converts a binary-coded decimal (BCD) value to an unsigned 8-bit integer.
 * @param val The BCD value to convert.
 * @return The converted unsigned 8-bit integer.
 */
uint8_t BcdToUint8(uint8_t val);

/**
 * @brief Converts an unsigned 8-bit integer to a binary-coded decimal (BCD) value.
 * @param val The unsigned 8-bit integer to convert.
 * @return The converted BCD value.
 */
uint8_t Uint8ToBcd(uint8_t val);

/**
 * @brief Converts a string to an unsigned 64-bit integer.
 * @param pString The string to convert.
 * @param _len The maximum length of the string (default is 32).
 * @return The converted unsigned 64-bit integer.
 */
uint64_t StringToUint64(const char *pString, uint8_t _len = 32);

/**
 * @brief Converts a string to an unsigned 32-bit integer.
 * @param pString The string to convert.
 * @param _len The maximum length of the string (default is 32).
 * @return The converted unsigned 32-bit integer.
 */
uint32_t StringToUint32(const char *pString, uint8_t _len = 32);

/**
 * @brief Converts a string to an unsigned 16-bit integer.
 * @param pString The string to convert.
 * @param _len The maximum length of the string (default is 32).
 * @return The converted unsigned 16-bit integer.
 */
uint16_t StringToUint16(const char *pString, uint8_t _len = 32);

/**
 * @brief Converts a string to an unsigned 8-bit integer.
 * @param pString The string to convert.
 * @param _len The maximum length of the string (default is 32).
 * @return The converted unsigned 8-bit integer.
 */
uint8_t StringToUint8(const char *pString, uint8_t _len = 32);

/**
 * @brief Converts a string to a hexadecimal 16-bit integer.
 * @param pString The string to convert.
 * @param _strlen The length of the string.
 * @return The converted hexadecimal 16-bit integer.
 */
uint16_t StringToHex16(const char *pString, uint8_t _strlen);

/**
 * @brief Converts a signed 32-bit integer to a string.
 * @param val The signed 32-bit integer to convert.
 * @param pString The buffer to store the resulting string.
 * @param _maxlen The maximum length of the string buffer.
 * @param _padmax The number of padding characters to add (default is 0).
 */
void Int32ToString(int32_t val, char *pString, uint8_t _maxlen, uint8_t _padmax = 0);

/**
 * @brief Converts a signed 64-bit integer to a string.
 * @param val The signed 64-bit integer to convert.
 * @param pString The buffer to store the resulting string.
 * @param _maxlen The maximum length of the string buffer.
 * @param _padmax The number of padding characters to add (default is 0).
 */
void Int64ToString(int64_t val, char *pString, uint8_t _maxlen, uint8_t _padmax = 0);

/**
 * @brief Counts the number of digits in a signed 32-bit integer.
 * @param x The signed 32-bit integer.
 * @return The number of digits in the integer.
 */
uint8_t Int32DigitCount(int32_t x);

/**
 * @brief Counts the number of digits in a signed 64-bit integer.
 * @param x The signed 64-bit integer.
 * @return The number of digits in the integer.
 */
uint8_t Int64DigitCount(int64_t x);

#endif
