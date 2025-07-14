/************************ Devices Common Config page **************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _DEVICES_COMMON_CONFIG_H_
#define _DEVICES_COMMON_CONFIG_H_

/**
 * @define task scheduler which makes use of timers
 */
// #define ENABLE_TIMER_TASK_SCHEDULER

/**
 * enable/disable devices. enable one from below list to get it compiled
 */
// #define MOCK_DEVICE_TEST
#define DEVICE_ESP8266
// #define DEVICE_ESP32
// #define DEVICE_ARDUINOUNO

/**
 * enable/disable gpio service here
 */
#define ENABLE_GPIO_SERVICE
#if defined(DEVICE_ARDUINOUNO)
#define ENABLE_GPIO_BASIC_ONLY
#endif

/**
 * enable/disable serial service
 */
#define ENABLE_SERIAL_SERVICE

/**
 * enable/disable storage service
 */
#if !defined(DEVICE_ARDUINOUNO)
#define ENABLE_STORAGE_SERVICE
#endif

/**
 * define number of gpios based on device enabled
 */
#if defined(DEVICE_ARDUINOUNO)
#define MAX_DIGITAL_GPIO_PINS         14
#define MAX_ANALOG_GPIO_PINS          5
#else
#define MAX_DIGITAL_GPIO_PINS         9
#define MAX_ANALOG_GPIO_PINS          1
#endif

/**
 * define max number of tables in database
 */
#if defined(DEVICE_ARDUINOUNO)
#define MAX_DB_TABLES 5
#else
#define MAX_DB_TABLES 15
#endif

/**
 * enable/disable network service here
 */
#if !defined(DEVICE_ARDUINOUNO)
#define ENABLE_NETWORK_SERVICE
#endif

/**
 * enable/disable auth service here
 */
#if !defined(DEVICE_ARDUINOUNO)
#define ENABLE_AUTH_SERVICE
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
 * enable/disable http server feature here
 */
#define ENABLE_HTTP_SERVER

/**
 * enable/disable http client
 */
#define ENABLE_HTTP_CLIENT

/**
 * enable/disable network subnetting ( dynamically set ap subnet,gateway etc. )
 */
#define ENABLE_DYNAMIC_SUBNETTING

/**
 * enable/disable internet availability based station connections
 */
// #define ENABLE_INTERNET_BASED_CONNECTIONS

/**
 * @define wifi & internet connectivity check cycle durations
 */
#define WIFI_STATION_CONNECT_ATTEMPT_TIMEOUT  5  // will try to connect within this seconds
#define WIFI_CONNECTIVITY_CHECK_DURATION      MILLISECOND_DURATION_10000
#define INTERNET_CONNECTIVITY_CHECK_DURATION  WIFI_CONNECTIVITY_CHECK_DURATION

/**
 * define connection switch duration once device recognise internet unavailability on current network
 * it should be multiple of INTERNET_CONNECTIVITY_CHECK_DURATION
 */
#ifdef ENABLE_INTERNET_BASED_CONNECTIONS
#define SWITCHING_DURATION_FOR_NO_INTERNET_CONNECTION INTERNET_CONNECTIVITY_CHECK_DURATION*6
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

#endif
