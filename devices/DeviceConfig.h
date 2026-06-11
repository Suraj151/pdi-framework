/************************ Devices Common Config page **************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _DEVICES_COMMON_CONFIG_H_
#define _DEVICES_COMMON_CONFIG_H_

#if __has_include("DeviceSetup.h")
#include "DeviceSetup.h"
#else
#define DEVICE_ESP32
#endif

/**
 * include device specific config if any
 */
#if defined(DEVICE_ESP8266)
#include "esp8266/esp8266_device_config.h"
#elif defined(DEVICE_ESP32)
#include "esp32/esp32_device_config.h"
#elif defined(DEVICE_ARDUINOUNO)
#include "arduinouno/arduinouno_device_config.h"
#else
#include "esp32/esp32_device_config.h"
#endif

/**
 * enable/disable gpio service here
 */
#define ENABLE_GPIO_SERVICE

/**
 * enable/disable serial service
 */
#define ENABLE_SERIAL_SERVICE

/**
 * enable/disable concurrency in task scheduling. By default kept disabled.
 * use only if you aware on the task context handling.
 *
 */
// #define ENABLE_CONTEXTUAL_EXECUTION
#if defined(ENABLE_CONTEXTUAL_EXECUTION) && !defined(DEVICE_SUPPORTS_CONTEXTUAL_EXECUTION)
#undef ENABLE_CONTEXTUAL_EXECUTION
#endif

/**
 * enable/disable tls service which provides the tls server and client instance to be use.
 * NOTE : tls service require more memory for its operations. which leaves minimal memory
 * to use it for app logic so keep this in mind while enabling the tls service.
 * By default kept disabled.
 */
// #define ENABLE_TLS_SERVICE
#if defined(ENABLE_TLS_SERVICE) && !defined(DEVICE_SUPPORTS_TLS)
#undef ENABLE_TLS_SERVICE
#endif
#if defined(ENABLE_TLS_SERVICE)
#define ENABLE_CONTEXTUAL_EXECUTION
#endif

/**
 * enable/disable tls certificate generation. Supported only on devices
 * that declare DEVICE_SUPPORTS_TLS_CERT_GENERATION.
 */
#if defined(ENABLE_TLS_SERVICE) && defined(DEVICE_SUPPORTS_TLS_CERT_GENERATION)
#define ENABLE_TLS_CERT_GENERATION
#endif

/**
 * Build-time gate for on-device generation of the HTTPS server certificate.
 * By default disabled.
 */
#ifdef ENABLE_TLS_CERT_GENERATION
// #define ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME
#endif

/**
 * enable/disable cmd service here
 */
#define ENABLE_CMD_SERVICE


#ifdef ENABLE_NETWORK_SERVICE

/**
 * enable/disable mqtt here
 */
#define ENABLE_MQTT_SERVICE

/**
 * enable/disable wifi feature here
 */
#define ENABLE_WIFI_SERVICE

/**
 * enable/disable telnet
 */
#define ENABLE_TELNET_SERVICE

/**
 * enable/disable ssh
 */
#if defined(ENABLE_STORAGE_SERVICE)
#define ENABLE_SSH_SERVICE
#endif

/**
 * enable/disable ota feature here
 */
#define ENABLE_OTA_SERVICE

/**
 * enable/disable email service here
 */
#define ENABLE_EMAIL_SERVICE

/**
 * enable/disable device iot feature here
 */
#define ENABLE_DEVICE_IOT

/**
 * ignore free relay connections created by same ssid
 */
#define IGNORE_FREE_RELAY_CONNECTIONS

/**
 * enable/disable http & https server feature here. by default https kept disabled.
 * you can enable it if required.
 * Note : make sure you make server certificates available at /etc/http/ filesystem path
 * of device to use https.
 */
#define ENABLE_HTTP_SERVER
#if defined(ENABLE_TLS_SERVICE)
// #define ENABLE_HTTPS_SERVER
#endif

/**
 * enable/disable http client
 */
#define ENABLE_HTTP_CLIENT

/**
 * enable/disable network subnetting ( dynamically set ap subnet,gateway etc. )
 */
#define ENABLE_DYNAMIC_SUBNETTING

/**
 * enable/disable NAPT. By default disabled.
 * Note : enabling this will increase heap memory consumption.
 * Recommended to disable in case if not required.
 *
 * esp8266 device specific note : Can not be used parallally in case if tls services
 * are enabled in devices like esp8266 where memory is limited.
 */
// #define ENABLE_NAPT

/**
 * enable/disable internet availability based station connections
 */
#define ENABLE_INTERNET_BASED_CONNECTIONS

/**
 * @define wifi & internet connectivity check cycle durations
 */
#define WIFI_STATION_CONNECT_ATTEMPT_TIMEOUT  1  // will try to connect within this seconds
#define WIFI_CONNECTIVITY_CHECK_DURATION      MILLISECOND_DURATION_5000
#define INTERNET_CONNECTIVITY_CHECK_DURATION  WIFI_CONNECTIVITY_CHECK_DURATION

/**
 * WiFi reconnect escalation tiers
 */
#define ALLOW_DEVICE_RESET_ON_WIFI_CONNECT_FAILURES

#define WIFI_RECONNECT_TIER1_DURATION   (MILLISECOND_DURATION_5000 * 3)    // 0 -  15 s
#define WIFI_RECONNECT_TIER2_DURATION   (MILLISECOND_DURATION_10000 * 6)   // 15 -  60 s
#define WIFI_RECONNECT_TIER3_DURATION   (MILLISECOND_DURATION_10000 * 12)  // 60 - 120 s
#ifdef ALLOW_DEVICE_RESET_ON_WIFI_CONNECT_FAILURES
#define WIFI_RECONNECT_TIER4_DURATION   (MILLISECOND_DURATION_10000 * 30)  // > 300 s. final restart device
#endif

#define WIFI_RECONNECT_TIER1_GAP        MILLISECOND_DURATION_5000
#define WIFI_RECONNECT_TIER2_GAP        MILLISECOND_DURATION_10000
#define WIFI_RECONNECT_TIER3_GAP        (MILLISECOND_DURATION_10000 * 3)
#define WIFI_RECONNECT_TIER4_GAP        (MILLISECOND_DURATION_10000 * 6)

/**
 * define connection switch duration once device recognise internet unavailability on current network
 * it should be at least WIFI_RECONNECT_TIER2_DURATION
 */
#ifdef ENABLE_INTERNET_BASED_CONNECTIONS
#define SWITCHING_DURATION_FOR_NO_INTERNET_CONNECTION WIFI_RECONNECT_TIER2_DURATION + WIFI_RECONNECT_TIER2_GAP
#endif


#endif

/**
 * enable/disable auto factory reset on invalid database config found
 */
#define AUTO_FACTORY_RESET_ON_INVALID_CONFIGS

/**
 * enable/disable config clear/reset on factory reset event
 */
#define CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET

/**
 * enable/disable logs here
 */
// #define ENABLE_LOG_ALL
// #define ENABLE_LOG_INFO
// #define ENABLE_LOG_WARNING
// #define ENABLE_LOG_ERROR
// #define ENABLE_LOG_SUCCESS

/**
 * consts
 */
#define NOT_APPLICABLE "NA"

#endif
