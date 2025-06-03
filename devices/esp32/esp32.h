/****************************** Common Config *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _ESP32_COMMON_CONFIG_H_
#define _ESP32_COMMON_CONFIG_H_

#include <Arduino.h>
#include <Esp.h>
#include "spi_flash_mmap.h"
#include "esp_rom_spiflash.h"
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_ping.h>
#include <ping/ping.h>
#include <esp_event.h>
#include <Print.h>
#include <SPIFFS.h>


#include <FS.h>
#include <Ticker.h>
#include <HTTPClient.h>
#include <IPAddress.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPUpdate.h>
#include <utility/DataTypeDef.h>
#include "esp_partition.h" // Include ESP32 partition API

#include <lwip/tcp.h>
#include <lwip/inet.h> // For IP address conversion

// extern "C"
// {
// // #include "user_interface.h"
// // #include <espnow.h>
// #include "esp_ping.h"
// }

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
 * todo : for now not using flash key pin feature
 */
#define FLASH_KEY_PIN -1
#define FLASH_KEY_PRESS_COUNT_THR 5

/**
 * enable/disable exception notifier
 */
// #define ENABLE_EXCEPTION_NOTIFIER

/**
 * enable/disable esp now feature here. currently this feature is not enabled yet for esp32
 */
// #define ENABLE_ESP_NOW

/**
 * @define network address & port translation feature
 */
// #if IP_NAPT && LWIP_VERSION_MAJOR == 1
//   #define ENABLE_NAPT_FEATURE
// #elif IP_NAPT && LWIP_VERSION_MAJOR >= 2
//   #define ENABLE_NAPT_FEATURE_LWIP_V2
// #endif


#define NAPT_INIT_DURATION_AFTER_WIFI_CONNECT MILLISECOND_DURATION_5000

#endif // _ESP32_COMMON_CONFIG_H_
