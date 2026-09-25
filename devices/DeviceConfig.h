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
 * MOCK_DEVICE_TEST is set on the compiler command line by the test build. It
 * selects the posix port and leaves the generated device setup untouched, so
 * the tests run against the same port a host build ships.
 */
#ifdef MOCK_DEVICE_TEST
#ifndef PDI_DEVICE
#define PDI_DEVICE posix
#endif
#endif

/**
 * PDI_DEVICE names the port directory under devices. Setting it on the compiler
 * command line selects a port without rewriting the generated device setup, so a
 * build matrix can cover every port from one tree.
 */
#ifndef PDI_DEVICE
#if __has_include("DeviceSetup.h")
#include "DeviceSetup.h"
#else
#define PDI_DEVICE esp32
#endif
#endif

/* expands PDI_DEVICE before stringizing it into a port relative include path */
#define PDI_PORT_STR(path) #path
#define PDI_PORT_PATH(path) PDI_PORT_STR(path)

/**
 * include device specific config if any. the port names itself by defining its
 * own DEVICE_* macro, so no selection chain grows as ports are added.
 */
#include PDI_PORT_PATH(PDI_DEVICE/device_config.h)

/**
 * the user store keeps its accounts in /etc/passwd and /etc/shadow, so auth is
 * only meaningful on a device that has the storage service.
 */
#if defined(ENABLE_AUTH_SERVICE) && !defined(ENABLE_STORAGE_SERVICE)
#undef ENABLE_AUTH_SERVICE
#endif

#ifndef TZ_Asia_Kolkata
#define TZ_Asia_Kolkata "IST-5:30"
#endif

#ifndef TZ
#define TZ              5.5    // (utc+) TZ in hours
#endif
#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)

#ifndef DST_MN
#define DST_MN          0      // use 60mn for summer time in some countries
#endif
#define DST_SEC         ((DST_MN)*60)

#ifndef NTP_SERVER1
#define NTP_SERVER1     "pool.ntp.org"
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
 * enable/disable dynamic program loading — load a compiled program from the
 * filesystem into RAM and run it at runtime. Each device maps this to its own
 * loadable-binary format (esp32: ELF). Supported only on devices that declare
 * DEVICE_SUPPORTS_PROGRAM_EXEC and with the storage service.
 */
#define ENABLE_PROGRAM_EXEC

#if defined(ENABLE_PROGRAM_EXEC) && (!defined(DEVICE_SUPPORTS_PROGRAM_EXEC) || !defined(ENABLE_STORAGE_SERVICE))
#undef ENABLE_PROGRAM_EXEC
#endif

/* enable/disable running a file of shell lines as a script */
#ifndef PDI_NO_SCRIPT_RUNNER
#define ENABLE_SCRIPT_RUNNER
#endif

#if defined(ENABLE_SCRIPT_RUNNER) && (!defined(ENABLE_CMD_SERVICE) || !defined(ENABLE_STORAGE_SERVICE))
#undef ENABLE_SCRIPT_RUNNER
#endif

/* enable/disable running commands from /etc/crontab on their scheduled minute */
#ifndef PDI_NO_CRON_SERVICE
#define ENABLE_CRON_SERVICE
#endif

#if defined(ENABLE_CRON_SERVICE) && (!defined(ENABLE_CMD_SERVICE) || !defined(ENABLE_STORAGE_SERVICE))
#undef ENABLE_CRON_SERVICE
#endif

/**
 * enable/disable sealing of the config records that hold a credential. A sealed
 * record is encrypted and carries a tag, under a key kept in the eeprom, so a
 * copy of the database taken off the device reveals nothing. Records that hold
 * no credential carry a checksum either way. Supported only on devices that
 * declare DEVICE_SUPPORTS_DB_SEALING, the ciphers and their state cost more ram
 * than a small device has to spare.
 */
#define ENABLE_DB_SEALING

#if defined(ENABLE_DB_SEALING) && !defined(DEVICE_SUPPORTS_DB_SEALING)
#undef ENABLE_DB_SEALING
#endif

/**
 * enable/disable concurrency in task scheduling. By default kept disabled.
 * use only if you aware on the task context handling.
 *
 */
// #define ENABLE_CONTEXTUAL_EXECUTION

#if defined(ENABLE_PROGRAM_EXEC)
#define ENABLE_CONTEXTUAL_EXECUTION
#endif
#if defined(ENABLE_CONTEXTUAL_EXECUTION) && !defined(DEVICE_SUPPORTS_CONTEXTUAL_EXECUTION)
#undef ENABLE_CONTEXTUAL_EXECUTION
#endif

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
 * enable/disable mdns responder here
 */
#define ENABLE_MDNS_SERVICE

/**
 * enable/disable telnet
 */
#define ENABLE_TELNET_SERVICE

/**
 * enable/disable ssh
 */
#if defined(ENABLE_STORAGE_SERVICE) && defined(ENABLE_AUTH_SERVICE)
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
#ifdef ENABLE_MQTT_SERVICE
// #define ENABLE_DEVICE_IOT
#endif

/**
 * ignore free relay connections created by same ssid
 */
#define IGNORE_FREE_RELAY_CONNECTIONS

/**
 * enable/disable http & https server feature here. by default https kept disabled.
 * as https may require more memory for encrypt/decrypt cycles it is disabled by default.
 * devices can enable https in their config by default if sufficient memory is available.
 */
#define ENABLE_HTTP_SERVER
// #define ENABLE_HTTPS_SERVER

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
#define INTERNET_CHECK_MAX_PING_BUSY_WAIT     (MILLISECOND_DURATION_10000*2)

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

/**
 * enable/disable tls service which provides the tls server and client instance to be use.
 * NOTE : tls service require more memory for its operations. which leaves minimal memory
 * to use it for app logic so keep this in mind while enabling the tls service.
 * By default kept disabled.
 */
// #define ENABLE_TLS_SERVICE

#if defined(ENABLE_HTTPS_SERVER)
#define ENABLE_TLS_SERVICE
#endif
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
 * enable/disable carrying a http request on a scheduled task instead of blocking
 * the caller. Supported only on devices that declare
 * DEVICE_SUPPORTS_OFFLOOP_NETWORK_TASK, elsewhere the request runs inline.
 */
#if defined(ENABLE_CONTEXTUAL_EXECUTION) && defined(DEVICE_SUPPORTS_OFFLOOP_NETWORK_TASK)
#define ENABLE_HTTP_CLIENT_ASYNC_REQUEST
#endif

/**
 * Build-time gate for on-device generation of the HTTPS server certificate.
 * By default enabled.
 */
#ifdef ENABLE_TLS_CERT_GENERATION
#define ENABLE_SERVER_TLS_CERT_GENERATION_AT_RUNTIME
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
 * enable/disable console (serial) logs here
 */
// #define ENABLE_CONSOLE_LOG_ALL
// #define ENABLE_CONSOLE_LOG_INFO
// #define ENABLE_CONSOLE_LOG_WARNING
// #define ENABLE_CONSOLE_LOG_ERROR
// #define ENABLE_CONSOLE_LOG_SUCCESS

/**
 * enable/disable syslog — persists log lines to /var/log/syslog.<type> files
 * via the LogManager service. Optional; requires the storage service. Use the
 * SysLog* macros for lines you want on console AND in the file.
 */
#if defined(ENABLE_STORAGE_SERVICE)
#define ENABLE_SYSLOG_SERVICE
#endif

/**
 * enable/disable remote syslog forwarding — ships SysLog* lines to a collector
 * (RFC 3164 over UDP). Requires syslog + network; set SYSLOG_REMOTE_HOST to the
 * collector address (empty leaves it compiled but idle).
 */
#if defined(ENABLE_SYSLOG_SERVICE) && defined(ENABLE_NETWORK_SERVICE)
// #define ENABLE_SYSLOG_FORWARD
#endif

/**
 * consts
 */
#define NOT_APPLICABLE "NA"

#endif
