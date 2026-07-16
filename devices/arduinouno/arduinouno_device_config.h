/*********************** ARDUINOUNO device Config *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2026
******************************************************************************/

#ifndef _ARDUINOUNO_DEVICE_CONFIG_H_
#define _ARDUINOUNO_DEVICE_CONFIG_H_

#include <Arduino.h>

#define RODT_ATTR(v) (const char*)F(v)
#define PROG_RODT_ATTR PROGMEM
#define PROG_RODT_PTR PGM_P

#define strcat_ro strcat_P
#define strncat_ro strncat_P
#define strcpy_ro strcpy_P
#define strncpy_ro strncpy_P
#define strlen_ro strlen_P
#define strcmp_ro strcmp_P
#define strncmp_ro strncmp_P
#define memcpy_ro memcpy_P

/**
 * gpio pin counts
 */
#define MAX_DIGITAL_GPIO_PINS         14
#define MAX_ANALOG_GPIO_PINS          5

/**
 * log2 might not available for uno
 */
#define log2(x) (log(x)/log(2.0))

/**
 * define max number of tables in database
 */
#define MAX_DB_TABLES 5

/**
 * enable basic gpio only on uno
 */
#define ENABLE_GPIO_BASIC_ONLY

/**
 * max tasks
 */
#define MAX_SCHEDULABLE_TASKS	8

#endif // _ARDUINOUNO_DEVICE_CONFIG_H_
