/************************ ESP32 device Config *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2026
******************************************************************************/

#ifndef _ESP32_DEVICE_CONFIG_H_
#define _ESP32_DEVICE_CONFIG_H_

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

#ifdef ENABLE_CONTEXTUAL_EXECUTION
extern portMUX_TYPE __pdi_critical_mux;
#define CRITICAL_SECTION_ENTER portENTER_CRITICAL(&__pdi_critical_mux);
#define CRITICAL_SECTION_EXIT  portEXIT_CRITICAL(&__pdi_critical_mux);
#else
#define CRITICAL_SECTION_ENTER noInterrupts();
#define CRITICAL_SECTION_EXIT  interrupts();
#endif

#define CMD_SIZE_MAX                8   ///< Maximum size of a command.
#define CMD_OPTION_MAX              6   ///< Maximum number of options for a command.
#define CMD_OPTION_SIZE_MAX         3   ///< Maximum size of an option.

/**
 * gpio pin counts
 */
#define MAX_DIGITAL_GPIO_PINS         9
#define MAX_ANALOG_GPIO_PINS          4

/**
 * define max number of tables in database
 */
#define MAX_DB_TABLES 15

/**
 * enable/disable storage service
 */
#define ENABLE_STORAGE_SERVICE

/**
 * enable/disable network service here
 */
#define ENABLE_NETWORK_SERVICE

/**
 * enable/disable auth service here
 */
#define ENABLE_AUTH_SERVICE

/**
 * device capabilities (read by common config to gate optional features)
 */
#define DEVICE_SUPPORTS_TLS
#define DEVICE_SUPPORTS_CONTEXTUAL_EXECUTION
#define DEVICE_SUPPORTS_TLS_CERT_GENERATION

/**
 * enable/disable ota upgrade strategies
 */
#define MAKE_STREAM_DIRECT_OTA_UPGRADE
// #ifdef ENABLE_STORAGE_SERVICE
// #define MAKE_STORAGE_DEPENDENT_OTA_UPGRADE
// #endif

#endif // _ESP32_DEVICE_CONFIG_H_
