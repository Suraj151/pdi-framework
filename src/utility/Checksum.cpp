/******************************* Checksum *************************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
Created Date    : 22nd Aug 2026
******************************************************************************/

#include "Checksum.h"

/**
 * continue a crc16 ccitt-false over a further byte range.
 *
 * @param   uint16_t        _crc
 * @param   const uint8_t*  _buf
 * @param   uint32_t        _len
 * @return  uint16_t
 */
uint16_t crc16CcittUpdate(uint16_t _crc, const uint8_t *_buf, uint32_t _len)
{
    for (uint32_t i = 0; i < _len; i++)
    {
        _crc ^= (uint16_t)((uint16_t)_buf[i] << 8);

        for (uint8_t b = 0; b < 8; b++)
        {
            _crc = (_crc & 0x8000) ? (uint16_t)((uint16_t)(_crc << 1) ^ 0x1021) : (uint16_t)(_crc << 1);
        }
    }

    return _crc;
}

/**
 * crc16 ccitt-false over a byte range.
 *
 * @param   const uint8_t*  _buf
 * @param   uint32_t        _len
 * @return  uint16_t
 */
uint16_t crc16Ccitt(const uint8_t *_buf, uint32_t _len)
{
    return crc16CcittUpdate(0xFFFF, _buf, _len);
}

/**
 * the 7 bit crc an sd card command or register carries.
 *
 * @param   const uint8_t*  _buf
 * @param   uint32_t        _len
 * @return  uint8_t
 */
uint8_t crc7(const uint8_t *_buf, uint32_t _len)
{
    uint8_t crc = 0;

    for (uint32_t i = 0; i < _len; i++)
    {
        uint8_t byte = _buf[i];

        for (uint8_t b = 0; b < 8; b++)
        {
            crc = (uint8_t)(crc << 1);
            if ((byte ^ crc) & 0x80)
            {
                crc ^= 0x09;
            }
            byte = (uint8_t)(byte << 1);
        }
    }

    return (uint8_t)(crc & 0x7F);
}
