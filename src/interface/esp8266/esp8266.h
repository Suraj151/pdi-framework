/****************************** Common Config *********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#ifndef _ESP8266_COMMON_CONFIG_H_
#define _ESP8266_COMMON_CONFIG_H_

#include <Arduino.h>
#include <Esp.h>
#include <Print.h>

#include <ESP8266HTTPClient.h>
#include <IPAddress.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <utility/DataTypeDef.h>

extern "C"
{
#include "user_interface.h"
#include <espnow.h>
#include <ping.h>
}

// redefines
#ifdef RODT_ATTR
#undef RODT_ATTR
#define RODT_ATTR(v) (const char*)F(v)
#endif


/**
 * @define flash key parameters for reset factory
 */
#define FLASH_KEY_PIN D3
#define FLASH_KEY_PRESS_COUNT_THR 5

/**
 * enable/disable esp now feature here
 */
// #define ENABLE_ESP_NOW

/**
 * @define network address & port translation feature
 */
#if IP_NAPT && LWIP_VERSION_MAJOR == 1
  #define ENABLE_NAPT_FEATURE
#elif IP_NAPT && LWIP_VERSION_MAJOR >= 2
  #define ENABLE_NAPT_FEATURE_LWIP_V2
#endif


#if defined( ENABLE_NAPT_FEATURE ) || defined( ENABLE_NAPT_FEATURE_LWIP_V2 )
  #define NAPT_INIT_DURATION_AFTER_WIFI_CONNECT MILLISECOND_DURATION_5000
#endif

#endif // _ESP8266_COMMON_CONFIG_H_
