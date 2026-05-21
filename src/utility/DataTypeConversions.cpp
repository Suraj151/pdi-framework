/************************* Data Type Converters *******************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#include "DataTypeConversions.h"

/**
 * @brief Converts a binary-coded decimal (BCD) value to an unsigned 8-bit integer.
 * 
 * This function converts a BCD value into its equivalent unsigned 8-bit integer.
 *
 * @param val The BCD value to convert.
 * @return The converted unsigned 8-bit integer.
 */
uint8_t BcdToUint8(uint8_t val)
{
    return val - 6 * (val >> 4);
}

/**
 * @brief Converts an unsigned 8-bit integer to a binary-coded decimal (BCD) value.
 * 
 * This function converts an unsigned 8-bit integer into its equivalent BCD value.
 *
 * @param val The unsigned 8-bit integer to convert.
 * @return The converted BCD value.
 */
uint8_t Uint8ToBcd(uint8_t val)
{
    return val + 6 * (val / 10);
}

/**
 * @brief Converts a string to an unsigned 64-bit integer.
 * 
 * This function parses a string and converts it into an unsigned 64-bit integer.
 *
 * @param pString The string to convert.
 * @param _len The maximum length of the string to parse (default is 32).
 * @return The converted unsigned 64-bit integer.
 */
uint64_t StringToUint64(const char *pString, uint8_t _len)
{
    if (nullptr == pString)
    {
        return 0;
    }

    uint64_t value = 0;
    uint8_t n = 0;

    while ((*pString == '0' || *pString == ' ' || *pString == '"') && n < _len)
    {
        pString++;
        n++;
    }

    while ((*pString >= '0' && *pString <= '9') && n < _len)
    {
        value *= 10;
        value += *pString - '0';
        pString++;
        n++;
    }
    return value;
}

/**
 * @brief Converts a string to an unsigned 32-bit integer.
 * 
 * This function parses a string and converts it into an unsigned 32-bit integer.
 *
 * @param pString The string to convert.
 * @param _len The maximum length of the string to parse (default is 32).
 * @return The converted unsigned 32-bit integer.
 */
uint32_t StringToUint32(const char *pString, uint8_t _len)
{
    if (nullptr == pString)
    {
        return 0;
    }

    uint32_t value = 0;
    uint8_t n = 0;

    while ((*pString == '0' || *pString == ' ' || *pString == '"') && n < _len)
    {
        pString++;
        n++;
    }

    while ((*pString >= '0' && *pString <= '9') && n < _len)
    {
        value *= 10;
        value += *pString - '0';
        pString++;
        n++;
    }
    return value;
}

/**
 * @brief Converts a string to an unsigned 16-bit integer.
 * 
 * This function parses a string and converts it into an unsigned 16-bit integer.
 *
 * @param pString The string to convert.
 * @param _len The maximum length of the string to parse (default is 32).
 * @return The converted unsigned 16-bit integer.
 */
uint16_t StringToUint16(const char *pString, uint8_t _len)
{
    if (nullptr == pString)
    {
        return 0;
    }

    uint16_t value = 0;
    uint8_t n = 0;

    while ((*pString == '0' || *pString == ' ' || *pString == '"') && n < _len)
    {
        pString++;
        n++;
    }

    while ((*pString >= '0' && *pString <= '9') && n < _len)
    {
        value *= 10;
        value += *pString - '0';
        pString++;
        n++;
    }
    return value;
}

/**
 * @brief Converts a string to an unsigned 8-bit integer.
 * 
 * This function parses a string and converts it into an unsigned 8-bit integer.
 *
 * @param pString The string to convert.
 * @param _len The maximum length of the string to parse (default is 32).
 * @return The converted unsigned 8-bit integer.
 */
uint8_t StringToUint8(const char *pString, uint8_t _len)
{
    if (nullptr == pString)
    {
        return 0;
    }

    uint8_t value = 0, n = 0;

    while ((*pString == '0' || *pString == ' ' || *pString == '"') && n < _len)
    {
        pString++;
        n++;
    }

    while ((*pString >= '0' && *pString <= '9') && n < _len)
    {
        value *= 10;
        value += *pString - '0';
        pString++;
        n++;
    }
    return value;
}

/**
 * @brief Converts a string to a hexadecimal 16-bit integer.
 * 
 * This function parses a string and converts it into a hexadecimal 16-bit integer.
 *
 * @param pString The string to convert.
 * @param _strlen The length of the string to parse.
 * @return The converted hexadecimal 16-bit integer.
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

    while ((*pString >= '0' && *pString <= '9') ||
           (*pString >= 'A' && *pString <= 'F') ||
           (*pString >= 'a' && *pString <= 'f'))
    {
        if (*pString >= 'A' && *pString <= 'F')
            value = 10 + (*pString - 'A');
        else if (*pString >= 'a' && *pString <= 'f')
            value = 10 + (*pString - 'a');
        else
            value = (*pString - '0');

        hexValue += hexWeight * value;
        hexWeight /= 16;
        pString++;
    }
    return hexValue;
}

/**
 * @brief Converts a signed 32-bit integer to a string.
 * 
 * This function converts a signed 32-bit integer into a string representation.
 *
 * @param val The signed 32-bit integer to convert.
 * @param pString The buffer to store the resulting string.
 * @param _maxlen The maximum length of the string buffer.
 * @param _padmax The number of padding characters to add (default is 0).
 */
void Int32ToString(int32_t val, char *pString, uint8_t _maxlen, uint8_t _padmax)
{
    if (nullptr == pString || _maxlen == 0) return;
    memset(pString, 0, _maxlen);

    // Reverse-build digits into a local buffer. No newlib usage so this is
    // safe to call from a preempted context.
    char tmp[12];                   // "-2147483648" + NUL fits
    int tpos = sizeof(tmp);
    bool neg = (val < 0);
    uint32_t u;
    if (val == INT32_MIN) {
        u = 2147483648u;            // |INT32_MIN| as unsigned
    } else {
        u = neg ? (uint32_t)(-val) : (uint32_t)val;
    }
    if (u == 0) {
        tmp[--tpos] = '0';
    } else {
        while (u > 0 && tpos > 0) {
            tmp[--tpos] = '0' + (u % 10);
            u /= 10;
        }
    }
    if (neg && tpos > 0) tmp[--tpos] = '-';

    int len = (int)sizeof(tmp) - tpos;
    if (len > _maxlen - 1) len = _maxlen - 1;
    memcpy(pString, &tmp[tpos], len);
    for (int l = len; l < _padmax && l < _maxlen - 1; l++)
        pString[l] = ' ';
}

/**
 * @brief Converts a signed 64-bit integer to a string.
 * 
 * This function converts a signed 64-bit integer into a string representation.
 *
 * @param val The signed 64-bit integer to convert.
 * @param pString The buffer to store the resulting string.
 * @param _maxlen The maximum length of the string buffer.
 * @param _padmax The number of padding characters to add (default is 0).
 */
void Int64ToString(int64_t val, char *pString, uint8_t _maxlen, uint8_t _padmax)
{
    if (nullptr == pString || _maxlen == 0) return;
    memset(pString, 0, _maxlen);

    char tmp[21];                   // "-9223372036854775808" + NUL fits
    int tpos = sizeof(tmp);
    bool neg = (val < 0);
    uint64_t u;
    if (val == INT64_MIN) {
        u = 9223372036854775808ull; // |INT64_MIN| as unsigned
    } else {
        u = neg ? (uint64_t)(-val) : (uint64_t)val;
    }
    if (u == 0) {
        tmp[--tpos] = '0';
    } else {
        while (u > 0 && tpos > 0) {
            tmp[--tpos] = '0' + (u % 10);
            u /= 10;
        }
    }
    if (neg && tpos > 0) tmp[--tpos] = '-';

    int len = (int)sizeof(tmp) - tpos;
    if (len > _maxlen - 1) len = _maxlen - 1;
    memcpy(pString, &tmp[tpos], len);
    for (int l = len; l < _padmax && l < _maxlen - 1; l++)
        pString[l] = ' ';
}

/**
 * @brief Converts an unsigned 32-bit integer to a decimal string.
 *
 * @param val The unsigned 32-bit integer to convert.
 * @param pString The buffer to store the resulting string.
 * @param _maxlen The maximum length of the string buffer.
 * @param _padmax The number of padding characters to add (default is 0).
 */
void Uint32ToString(uint32_t val, char *pString, uint8_t _maxlen, uint8_t _padmax)
{
    if (nullptr == pString || _maxlen == 0) return;
    memset(pString, 0, _maxlen);

    char tmp[11];                   // "4294967295" + NUL fits
    int tpos = sizeof(tmp);
    if (val == 0) {
        tmp[--tpos] = '0';
    } else {
        while (val > 0 && tpos > 0) {
            tmp[--tpos] = '0' + (val % 10);
            val /= 10;
        }
    }

    int len = (int)sizeof(tmp) - tpos;
    if (len > _maxlen - 1) len = _maxlen - 1;
    memcpy(pString, &tmp[tpos], len);
    for (int l = len; l < _padmax && l < _maxlen - 1; l++)
        pString[l] = ' ';
}

/**
 * @brief Converts an unsigned 32-bit integer to a hexadecimal string (no "0x" prefix).
 *
 * @param val The unsigned 32-bit integer to convert.
 * @param pString The buffer to store the resulting string.
 * @param _maxlen The maximum length of the string buffer.
 * @param cap If true, uses uppercase A-F.
 */
void Uint32ToHexString(uint32_t val, char *pString, uint8_t _maxlen, bool cap)
{
    if (nullptr == pString || _maxlen == 0) return;
    memset(pString, 0, _maxlen);

    char tmp[9];                    // "FFFFFFFF" + NUL fits
    int tpos = sizeof(tmp);
    const char base = cap ? 'A' : 'a';
    if (val == 0) {
        tmp[--tpos] = '0';
    } else {
        while (val > 0 && tpos > 0) {
            uint32_t n = val & 0xF;
            tmp[--tpos] = (n < 10) ? ('0' + n) : (base + (n - 10));
            val >>= 4;
        }
    }

    int len = (int)sizeof(tmp) - tpos;
    if (len > _maxlen - 1) len = _maxlen - 1;
    memcpy(pString, &tmp[tpos], len);
}

/**
 * @brief Converts a floating-point value to a string.
 *
 * @param val The floating-point value to convert.
 * @param pString The buffer to store the resulting string.
 * @param _maxlen The maximum length of the string buffer.
 */
void FloatToString(double val, char *pString, uint8_t _maxlen, uint8_t _padmax)
{
    if (nullptr == pString || _maxlen == 0) return;
    memset(pString, 0, _maxlen);

    // Match the default "%f" form: fixed 6 decimal places, leading sign for
    // negatives, no scientific notation. Intentionally simple — no NaN/Inf
    // handling (callers are expected to filter those).
    bool neg = (val < 0.0);
    if (neg) val = -val;

    uint64_t int_part = (uint64_t)val;
    double frac = val - (double)int_part;
    uint64_t frac_part = (uint64_t)(frac * 1000000.0 + 0.5);
    if (frac_part >= 1000000ull) { frac_part -= 1000000ull; int_part += 1; }

    int pos = 0;
    if (neg && pos < _maxlen - 1) pString[pos++] = '-';

    // Integer part via reverse-digit buffer
    char tmp[21];
    int tpos = sizeof(tmp);
    if (int_part == 0) {
        tmp[--tpos] = '0';
    } else {
        while (int_part > 0 && tpos > 0) {
            tmp[--tpos] = '0' + (int_part % 10);
            int_part /= 10;
        }
    }
    int ilen = (int)sizeof(tmp) - tpos;
    if (pos + ilen > _maxlen - 1) ilen = _maxlen - 1 - pos;
    if (ilen > 0) { memcpy(pString + pos, &tmp[tpos], ilen); pos += ilen; }

    if (pos < _maxlen - 1) pString[pos++] = '.';

    // Fixed 6 fractional digits, leading zeros preserved
    char fbuf[6];
    for (int i = 5; i >= 0; i--) {
        fbuf[i] = '0' + (frac_part % 10);
        frac_part /= 10;
    }
    int flen = 6;
    if (pos + flen > _maxlen - 1) flen = _maxlen - 1 - pos;
    if (flen > 0) { memcpy(pString + pos, fbuf, flen); pos += flen; }

    for (int l = pos; l < _padmax && l < _maxlen - 1; l++) pString[l] = ' ';
}

/**
 * @brief Counts the number of digits in a signed 32-bit integer.
 *
 * This function calculates the number of digits in a signed 32-bit integer.
 *
 * @param x The signed 32-bit integer.
 * @return The number of digits in the integer.
 */
uint8_t Int32DigitCount(int32_t x)
{
    if (x == INT32_MIN)
        return 10 + 1;
    if (x < 0)
        return Int32DigitCount(-x) + 1;

    if (x >= 10000)
    {
        if (x >= 10000000)
        {
            if (x >= 100000000)
            {
                if (x >= 1000000000)
                    return 10;
                return 9;
            }
            return 8;
        }
        if (x >= 100000)
        {
            if (x >= 1000000)
                return 7;
            return 6;
        }
        return 5;
    }
    if (x >= 100)
    {
        if (x >= 1000)
            return 4;
        return 3;
    }
    if (x >= 10)
        return 2;
    return 1;
}

/**
 * @brief Counts the number of digits in a signed 64-bit integer.
 * 
 * This function calculates the number of digits in a signed 64-bit integer.
 *
 * @param x The signed 64-bit integer.
 * @return The number of digits in the integer.
 */
uint8_t Int64DigitCount(int64_t x)
{
    if (x == INT64_MIN)
        return 19 + 1;
    if (x < 0)
        return Int64DigitCount(-x) + 1;

    if (x >= 10000000000)
    {
        if (x >= 100000000000000)
        {
            if (x >= 10000000000000000)
            {
                if (x >= 100000000000000000)
                {
                    if (x >= 1000000000000000000)
                        return 19;
                    return 18;
                }
                return 17;
            }
            if (x >= 1000000000000000)
                return 16;
            return 15;
        }
        if (x >= 1000000000000)
        {
            if (x >= 10000000000000)
                return 14;
            return 13;
        }
        if (x >= 100000000000)
            return 12;
        return 11;
    }
    if (x >= 100000)
    {
        if (x >= 10000000)
        {
            if (x >= 100000000)
            {
                if (x >= 1000000000)
                    return 10;
                return 9;
            }
            return 8;
        }
        if (x >= 1000000)
            return 7;
        return 6;
    }
    if (x >= 100)
    {
        if (x >= 1000)
        {
            if (x >= 10000)
                return 5;
            return 4;
        }
        return 3;
    }
    if (x >= 10)
        return 2;
    return 1;
}