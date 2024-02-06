/*************************** Common Config page *******************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/
#ifndef _COMMON_CONFIG_H_
#define _COMMON_CONFIG_H_

#include <utility/DataTypeDef.h>

/**
 * @define common time durations
 */
#define MILLISECOND_DURATION_1000   1000
#define MILLISECOND_DURATION_5000   5000
#define MILLISECOND_DURATION_10000  10000


/**
 * @define task scheduler which makes use of timers
 */
// #define ENABLE_TIMER_TASK_SCHEDULER

/**
 * enable/disable gpio and mqtt feature here
 */
#define ENABLE_MQTT_SERVICE
#define ENABLE_GPIO_SERVICE

/**
 * enable/disable email service here
 */
#define ENABLE_EMAIL_SERVICE

/**
 * enable/disable device iot feature here
 */
#define ENABLE_DEVICE_IOT

/**
 * enable/disable logging
 */
#define ENABLE_INFO_LOG
#define ENABLE_ERROR_LOG
#define ENABLE_WARNING_LOG
#define ENABLE_SUCCESS_LOG

/**
 * enable/disable auto factory reset on invalid database config found
 */
#define AUTO_FACTORY_RESET_ON_INVALID_CONFIGS

/**
 * enable/disable exception notifier
 */
#define ENABLE_EXCEPTION_NOTIFIER

/**
 * ignore free relay connections created by same ssid
 */
#define IGNORE_FREE_RELAY_CONNECTIONS

/**
 * enable/disable http server feature here
 */
#define ENABLE_EWING_HTTP_SERVER

/**
 * enable/disable network subnetting ( dynamically set ap subnet,gateway etc. )
 */
#define ENABLE_DYNAMIC_SUBNETTING

/**
 * enable/disable internet availability based station connections
 */
#define ENABLE_INTERNET_BASED_CONNECTIONS

/**
 * enable/disable esp now feature here
 */
#define ENABLE_ESP_NOW

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

/**
 * @define default username/ssid and password
 */
#define USER            "esp8266Stack"
#define PASSPHRASE      "espStack@8266"

/**
 * @define general http parameters
 */
#define HTTP_HOST_ADDR_MAX_SIZE 100
#define HTTP_REQUEST_DURATION   MILLISECOND_DURATION_10000
#define HTTP_REQUEST_RETRY      1

/**
 * max tasks and callbacks
 */
#define MAX_SCHEDULABLE_TASKS	25
#define MAX_FACTORY_RESET_CALLBACKS	MAX_SCHEDULABLE_TASKS
#define MAX_EVENT_CALLBACK_TASKS	MAX_SCHEDULABLE_TASKS

#endif
