/*********************** ARDUINOMEGA device Config *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2026
******************************************************************************/

#ifndef _ARDUINOMEGA_DEVICE_CONFIG_H_
#define _ARDUINOMEGA_DEVICE_CONFIG_H_

/* the port names itself so the framework can test for it without a selection chain */
#define DEVICE_ARDUINOMEGA

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
#define MAX_DIGITAL_GPIO_PINS         16
#define MAX_ANALOG_GPIO_PINS          8

/**
 * log2 might not available for avr
 */
#define log2(x) (log(x)/log(2.0))

/**
 * define max number of tables in database
 */
#define MAX_DB_TABLES 5

/**
 * enable basic gpio only on avr
 */
#define ENABLE_GPIO_BASIC_ONLY

/**
 * max tasks
 */
#define MAX_SCHEDULABLE_TASKS	16

/**
 * enable/disable command line service, the shell over the serial console
 */
#define ENABLE_CMD_SERVICE

#endif // _ARDUINOMEGA_DEVICE_CONFIG_H_
