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
#include <HTTPUpdate.h>
#include <utility/DataTypeDef.h>
#include "esp_partition.h" // Include ESP32 partition API

#include <lwip/tcp.h>
#include <lwip/inet.h> // For IP address conversion
#include <lwip/opt.h>
#include <lwip/tcpip.h>
#include "lwip/dns.h"
#include "netdb.h"

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

#ifdef PROG_RODT_PTR
#undef PROG_RODT_PTR
#define PROG_RODT_PTR PGM_P
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

#ifdef memcpy_ro
#undef memcpy_ro
#define memcpy_ro memcpy_P
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

#ifdef LOCK_TCPIP_CORE
#define LOCK_LWIP_TCPIP_CORE LOCK_TCPIP_CORE()
#else
#define LOCK_LWIP_TCPIP_CORE ((void)0)
#endif

#ifdef UNLOCK_TCPIP_CORE
#define UNLOCK_LWIP_TCPIP_CORE UNLOCK_TCPIP_CORE()
#else
#define UNLOCK_LWIP_TCPIP_CORE ((void)0)
#endif

// Value-returning wrapper
#define TCP_LOCKED_CALL_RETVAL(expr)                               \
    ({                                                             \
        bool _need_lock = !sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER); \
        if (_need_lock) { LOCK_LWIP_TCPIP_CORE; }                  \
        auto _res = (expr);                                        \
        if (_need_lock) { UNLOCK_LWIP_TCPIP_CORE; }                \
        _res;                                                      \
    })

// Void-returning wrapper
#define TCP_LOCKED_CALL_RETVOID(expr)                              \
    do {                                                           \
        bool _need_lock = !sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER); \
        if (_need_lock) { LOCK_LWIP_TCPIP_CORE; }                  \
        (expr);                                                    \
        if (_need_lock) { UNLOCK_LWIP_TCPIP_CORE; }                \
    } while (0)

// Now redefine the functions you use
#define tcp_new()                 TCP_LOCKED_CALL_RETVAL(tcp_new())
#define tcp_bind(pcb, ip, port)   TCP_LOCKED_CALL_RETVAL(tcp_bind(pcb, ip, port))
#define tcp_listen(pcb)           TCP_LOCKED_CALL_RETVAL(tcp_listen_with_backlog(pcb, TCP_DEFAULT_LISTEN_BACKLOG))
#define tcp_accept(pcb, accept)   TCP_LOCKED_CALL_RETVOID(tcp_accept(pcb, accept))
#define tcp_arg(pcb, arg)         TCP_LOCKED_CALL_RETVOID(tcp_arg(pcb, arg))
#define tcp_sent(pcb, arg)         TCP_LOCKED_CALL_RETVOID(tcp_sent(pcb, arg))
#define tcp_recv(pcb, arg)         TCP_LOCKED_CALL_RETVOID(tcp_recv(pcb, arg))
#define tcp_err(pcb, arg)         TCP_LOCKED_CALL_RETVOID(tcp_err(pcb, arg))
#define tcp_write(pcb, data, len, flags) TCP_LOCKED_CALL_RETVAL(tcp_write(pcb, data, len, flags))
#define tcp_output(pcb)           TCP_LOCKED_CALL_RETVAL(tcp_output(pcb))
#define tcp_recved(pcb, len)      TCP_LOCKED_CALL_RETVOID(tcp_recved(pcb, len))
#define tcp_close(pcb)            TCP_LOCKED_CALL_RETVAL(tcp_close(pcb))
#define tcp_abort(pcb)            TCP_LOCKED_CALL_RETVOID(tcp_abort(pcb))
#define tcp_connect(pcb, ip, port, connected) TCP_LOCKED_CALL_RETVAL(tcp_connect(pcb, ip, port, connected))

#define NAPT_INIT_DURATION_AFTER_WIFI_CONNECT MILLISECOND_DURATION_5000

#endif // _ESP32_COMMON_CONFIG_H_
