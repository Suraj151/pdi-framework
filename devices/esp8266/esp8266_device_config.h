/**************************** ESP8266 device Config ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2026
******************************************************************************/

#ifndef _ESP8266_DEVICE_CONFIG_H_
#define _ESP8266_DEVICE_CONFIG_H_

#include <Arduino.h>


#define RODT_ATTR(v) (const char*)F(v)
#define PROG_RODT_ATTR PROGMEM
#define PROG_RODT_PTR PGM_P

#define CRITICAL_SECTION_ENTER noInterrupts();
#define CRITICAL_SECTION_EXIT interrupts();

#define strcat_ro strcat_P
#define strncat_ro strncat_P
#define strcpy_ro strcpy_P
#define strncpy_ro strncpy_P
#define strlen_ro strlen_P
#define strcmp_ro strcmp_P
#define strncmp_ro strncmp_P
#define memcpy_ro memcpy_P


#endif // _ESP8266_DEVICE_CONFIG_H_
