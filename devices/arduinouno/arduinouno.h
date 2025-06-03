/****************************** Common Config *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _ARDUINOUNO_COMMON_CONFIG_H_
#define _ARDUINOUNO_COMMON_CONFIG_H_

#include <Arduino.h>
#include <Print.h>
#include <utility/DataTypeDef.h>
#include <avr/common.h>

// redefines
#ifdef RODT_ATTR
#undef RODT_ATTR
#define RODT_ATTR(v) (const char*)F(v)
#endif

#ifdef PROG_RODT_ATTR
#undef PROG_RODT_ATTR
#define PROG_RODT_ATTR PROGMEM
#endif

#ifdef strcat_ro
#undef strcat_ro
#define strcat_ro strcat_P
#endif

#ifdef strncat_ro
#undef strncat_ro
#define strncat_ro strncat_P
#endif

#ifdef strcpy_ro
#undef strcpy_ro
#define strcpy_ro strcpy_P
#endif

#ifdef strncpy_ro
#undef strncpy_ro
#define strncpy_ro strncpy_P
#endif

#ifdef strlen_ro
#undef strlen_ro
#define strlen_ro strlen_P
#endif

#ifdef strcmp_ro
#undef strcmp_ro
#define strcmp_ro strcmp_P
#endif

#ifdef strncmp_ro
#undef strncmp_ro
#define strncmp_ro strncmp_P
#endif


/**
 * @define flash key parameters for reset factory
 */
#define FLASH_KEY_PIN 0
#define FLASH_KEY_PRESS_COUNT_THR 5


#endif // _ARDUINOUNO_COMMON_CONFIG_H_
