/*************************** Mock device Config *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _MOCKDEVICE_DEVICE_CONFIG_H_
#define _MOCKDEVICE_DEVICE_CONFIG_H_

#include <string.h>

/**
 * the host keeps read only data in the same address space as everything else,
 * so the flash accessors collapse to their plain counterparts
 */
#define RODT_ATTR(v) (const char *)(v)
#define PROG_RODT_ATTR
#define PROG_RODT_PTR const char *

#define CRITICAL_SECTION_ENTER
#define CRITICAL_SECTION_EXIT

#define NESTED_CRITICAL_SECTION_ENTER
#define NESTED_CRITICAL_SECTION_EXIT

#define strcat_ro strcat
#define strncat_ro strncat
#define strcpy_ro strcpy
#define strncpy_ro strncpy
#define strlen_ro strlen
#define strcmp_ro strcmp
#define strncmp_ro strncmp
#define memcpy_ro memcpy

/**
 * gpio pin counts
 */
#define MAX_DIGITAL_GPIO_PINS 16
#define MAX_ANALOG_GPIO_PINS 2

/**
 * define max number of tables in database
 */
#define MAX_DB_TABLES 15

/**
 * services this device offers. each one is turned on as its host backend lands.
 */
#define ENABLE_STORAGE_SERVICE
#define ENABLE_NETWORK_SERVICE
#define ENABLE_AUTH_SERVICE
#define ENABLE_CMD_SERVICE

#endif // _MOCKDEVICE_DEVICE_CONFIG_H_
