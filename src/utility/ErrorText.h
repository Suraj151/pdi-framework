/******************************* Error Text *************************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The words a person reads for a pdi_err_t code, so every command, page and log
names a failure the same way instead of showing its number.

Author          : Suraj I.
Created Date    : 5th Nov 2026
******************************************************************************/

#ifndef _ERROR_TEXT_H_
#define _ERROR_TEXT_H_

#include "DataTypeDef.h"

/**
 * @brief The words for an error code: the generic codes, the storage codes and
 *        the LittleFS and FatFs codes carried through.
 * @param code The code.
 * @return Read only text, or nullptr for a code that has no words yet.
 */
const char *ErrorToText(int32_t code);

#endif
