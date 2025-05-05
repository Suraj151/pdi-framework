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

/**
 * @define flash key parameters for reset factory
 */
#define FLASH_KEY_PIN 0
#define FLASH_KEY_PRESS_COUNT_THR 5


#endif // _ARDUINOUNO_COMMON_CONFIG_H_
