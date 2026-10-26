/******************************* Checksum *************************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

Integrity checksums shared by every use site that has to detect a byte range
changing underneath it, such as stored records, transferred blocks or headers.

Author          : Suraj I.
Created Date    : 22nd Aug 2026
******************************************************************************/

#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_

#include "DataTypeDef.h"

/**
 * @brief crc16 ccitt-false over a byte range.
 *
 * Seeded with 0xFFFF so a range of zero bytes still produces a changing value.
 *
 * @param _buf Bytes to run over.
 * @param _len Number of bytes.
 * @return The checksum of the range.
 */
uint16_t crc16Ccitt(const uint8_t *_buf, uint32_t _len);

/**
 * @brief Continue a crc16 ccitt-false over a further byte range.
 *
 * Lets a checksum span buffers that are not contiguous in memory.
 *
 * @param _crc Checksum so far, 0xFFFF to start a fresh one.
 * @param _buf Bytes to run over.
 * @param _len Number of bytes.
 * @return The checksum including the range.
 */
uint16_t crc16CcittUpdate(uint16_t _crc, const uint8_t *_buf, uint32_t _len);

/**
 * @brief The 7 bit crc an SD card command or register carries, polynomial
 * x^7 + x^3 + 1 seeded with zero.
 *
 * @param _buf Bytes to run over.
 * @param _len Number of bytes.
 * @return The checksum in the low 7 bits.
 */
uint8_t crc7(const uint8_t *_buf, uint32_t _len);

#endif
