/*************************** Mock device Config *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _PDI_POSIX_DEVICE_CONFIG_H_
#define _PDI_POSIX_DEVICE_CONFIG_H_

/* the port names itself so the framework can test for it without a selection chain */
#define DEVICE_POSIX

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

#define CMD_SIZE_MAX                16  /* longest command name plus its terminator */
#define CMD_OPTION_MAX              6   /* most options one command takes */

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
 * device can spare the ram the record ciphers need
 */
#ifndef PDI_NO_DB_SEALING
#define DEVICE_SUPPORTS_DB_SEALING
#endif

#define DEVICE_SUPPORTS_NTP

#define DEVICE_SUPPORTS_DRIVER_CORE
#define DEVICE_SUPPORTS_I2S
#define DEVICE_SUPPORTS_I2C
#define DEVICE_SUPPORTS_SPI

/**
 * services this device offers. each one is turned on as its host backend lands.
 */
#ifndef PDI_NO_STORAGE_SERVICE
#define ENABLE_STORAGE_SERVICE
#endif
#ifndef PDI_NO_NETWORK_SERVICE
#define ENABLE_NETWORK_SERVICE
#endif
#ifndef PDI_NO_AUTH_SERVICE
#define ENABLE_AUTH_SERVICE
#endif
#ifndef PDI_NO_CMD_SERVICE
#define ENABLE_CMD_SERVICE
#endif

/**
 * feature settings files this device keeps under /etc. a host has no block
 * budget to protect, so every one of them is on and stays covered by the tests.
 */
#if !defined(PDI_NO_STORAGE_SERVICE) && !defined(PDI_NO_FEATURE_CONFIG_FILES)
#define ENABLE_WIFI_CONFIG_FILE
#define ENABLE_MQTT_CONFIG_FILE
#define ENABLE_OTA_CONFIG_FILE
#define ENABLE_EMAIL_CONFIG_FILE
#endif

#endif // _PDI_POSIX_DEVICE_CONFIG_H_
