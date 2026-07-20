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
 * @brief Converts an octal string (leading '0' optional) to a 16-bit value.
 *        Digits outside 0-7 terminate parsing.
 * @param pString The string to convert.
 * @param _len The maximum length of the string.
 * @return The parsed 16-bit octal value.
 */
uint16_t StringToOctalUint16(const char *pString, uint8_t _len);

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
 * @brief Converts an unsigned 32-bit integer to a decimal string.
 * @param val The unsigned 32-bit integer to convert.
 * @param pString The buffer to store the resulting string.
 * @param _maxlen The maximum length of the string buffer.
 * @param _padmax The number of padding characters to add (default is 0).
 */
void Uint32ToString(uint32_t val, char *pString, uint8_t _maxlen, uint8_t _padmax = 0);

/**
 * @brief Converts an unsigned 32-bit integer to a hexadecimal string (no "0x" prefix).
 * @param val The unsigned 32-bit integer to convert.
 * @param pString The buffer to store the resulting string.
 * @param _maxlen The maximum length of the string buffer.
 * @param cap If true, uses uppercase A-F (default is false).
 */
void Uint32ToHexString(uint32_t val, char *pString, uint8_t _maxlen, bool cap = false);

/**
 * @brief Converts a byte array to a lowercase hex string.
 * @param bytes Source byte array.
 * @param bytelen Number of bytes to encode.
 * @param out Destination buffer, must hold at least (bytelen*2 + 1) bytes.
 */
void BytesToHexString(const uint8_t *bytes, uint8_t bytelen, char *out);

/**
 * @brief Parses a hex string into a byte array.
 * @param hex Source hex string (2*bytelen characters, upper or lower case).
 * @param bytelen Number of bytes to decode.
 * @param out Destination byte array of size bytelen.
 * @return true on success, false if any character was not a hex digit.
 */
bool HexStringToBytes(const char *hex, uint8_t bytelen, uint8_t *out);

/**
 * @brief Converts a floating-point value to a string.
 * @param val The floating-point value to convert.
 * @param pString The buffer to store the resulting string.
 * @param _maxlen The maximum length of the string buffer.
 * @param _padmax The number of padding characters to add (default is 0).
 */
void FloatToString(double val, char *pString, uint8_t _maxlen, uint8_t _padmax = 0);

/**
 * @brief 3-letter English month abbreviations, indexed 1..12
 *        (slot 0 is a blank sentinel so month arithmetic reads naturally).
 *        Sits in .rodata; single copy shared across the whole build.
 */
extern const char __g_month_abbr[13][4];

/**
 * @brief Converts a Unix epoch (seconds since 1970-01-01 UTC) into a
 *        human-readable string using a strftime-style format.
 *        For local time, add the desired offset in seconds before calling.
 *        An epoch value of 0 is treated as "unknown" and written as a dash
 *        padded (with spaces) to the width the format would have produced.
 *
 *        Supported specifiers:
 *          %Y - 4-digit year        %y - 2-digit year
 *          %m - 2-digit month       %b - 3-letter month name (Jan..Dec)
 *          %d - 2-digit day         %H - 2-digit hour (24h)
 *          %M - 2-digit minute      %S - 2-digit second
 *          %% - literal '%'
 *        Any other characters (including unknown '%X') are copied verbatim.
 *
 * @param epoch  Unix epoch seconds to format.
 * @param pString Output buffer.
 * @param _maxlen Capacity of the output buffer (including the NUL terminator).
 * @param fmt    Format string. Defaults to "%Y-%m-%d %H:%M:%S" (19 chars + NUL).
 */
void EpochToDateTimeString(uint32_t epoch, char *pString, uint8_t _maxlen,
                           const char *fmt = "%Y-%m-%d %H:%M:%S");

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
