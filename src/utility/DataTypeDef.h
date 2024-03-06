/******************************** type defs ***********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _DATA_TYPE_DEFS_H_
#define _DATA_TYPE_DEFS_H_

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string>
#include <cstdio>
#include <cstring>
#include <functional>

// option to define the attribute for read only data
// redefine this in derived interface
#define RODT_ATTR(x) x

typedef uint8_t gpio_id_t;
typedef int64_t gpio_val_t;

typedef uint8_t wifi_mode_t;
typedef uint8_t sleep_mode_t;
typedef uint16_t wifi_status_t;
typedef uint8_t follow_redirects_t;

typedef uint16_t http_method_t;

/**
 * Define required callback type
 */
typedef std::function<void(int)> CallBackIntArgFn;
typedef std::function<void(void)> CallBackVoidArgFn;
typedef std::function<void(void *)> CallBackVoidPointerArgFn;

/**
 * Define log types
 */
typedef enum
{
  INFO_LOG,
  ERROR_LOG,
  WARNING_LOG,
  SUCCESS_LOG
} logger_type_t;

/**
 * Define ip stuff
 */
typedef enum
{
  IP_ADDR_TYPE_V4 = 4,  /** IPv4 */
  IP_ADDR_TYPE_V6 = 6,  /** IPv6 */
  IP_ADDR_TYPE_ANY = 46 /** IPv4+IPv6 ("dual-stack") */
} ip_addr_type;

struct ipaddress_t
{
  uint8_t ip4[4];
  ip_addr_type type;

  ipaddress_t(uint32_t _ip4) : type(IP_ADDR_TYPE_V4)
  {
    ip4[0] = (uint8_t)_ip4;
    ip4[1] = (uint8_t)(_ip4 >> 8);
    ip4[2] = (uint8_t)(_ip4 >> 16);
    ip4[3] = (uint8_t)(_ip4 >> 24);
  }

  ipaddress_t(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet) : type(IP_ADDR_TYPE_V4)
  {
    ip4[0] = first_octet;
    ip4[1] = second_octet;
    ip4[2] = third_octet;
    ip4[3] = fourth_octet;
  }

  operator std::string()
  {
    return (std::to_string(ip4[0]) + "." + std::to_string(ip4[1]) + "." + std::to_string(ip4[2]) + "." + std::to_string(ip4[3]));
  }

  operator uint32_t()
  {
    return (((uint32_t)ip4[0]<<24) | ((uint32_t)ip4[1]<<16) | ((uint32_t)ip4[2]<<8) | ((uint32_t)ip4[3]));
  }

  uint8_t operator[](int index)
  {
    return index < 4 ? ip4[index] : 0;
  }
};


/* Http defs */
typedef enum {
    HTTP_RESP_OK = 200,
    HTTP_RESP_MULTIPLE_CHOICES = 300,
    HTTP_RESP_MOVED_PERMANENTLY = 301,
    HTTP_RESP_FOUND = 302,
    HTTP_RESP_SEE_OTHER = 303,
    HTTP_RESP_NOT_MODIFIED = 304,
    HTTP_RESP_USE_PROXY = 305,
    HTTP_RESP_TEMPORARY_REDIRECT = 307,
    HTTP_RESP_PERMANENT_REDIRECT = 308,
    HTTP_RESP_BAD_REQUEST = 400,
    HTTP_RESP_UNAUTHORIZED = 401,
    HTTP_RESP_PAYMENT_REQUIRED = 402,
    HTTP_RESP_FORBIDDEN = 403,
    HTTP_RESP_NOT_FOUND = 404,
    HTTP_RESP_METHOD_NOT_ALLOWED = 405,
    HTTP_RESP_REQUEST_TIMEOUT = 408,
    HTTP_RESP_INTERNAL_SERVER_ERROR = 500,
    HTTP_RESP_NOT_IMPLEMENTED = 501,
    HTTP_RESP_BAD_GATEWAY = 502,
    HTTP_RESP_SERVICE_UNAVAILABLE = 503,
    HTTP_RESP_GATEWAY_TIMEOUT = 504,
    HTTP_RESP_HTTP_VERSION_NOT_SUPPORTED = 505,
    HTTP_RESP_VARIANT_ALSO_NEGOTIATES = 506,
    HTTP_RESP_MAX = 999
} http_resp_code_t;

typedef enum {
    HTTP_ERROR_CONNECTION_FAILED = -1,
    HTTP_ERROR_MAX = -999
} http_err_code_t;

typedef enum {
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
    HTTP_VERSION_2,
    HTTP_VERSION_3,
    HTTP_VERSION_MAX
} http_version_t;

/* upgrade defs */
typedef enum {
    UPGRADE_STATUS_FAILED = -1,
    UPGRADE_STATUS_SUCCESS,
    UPGRADE_STATUS_IGNORE,  // no update available
    UPGRADE_STATUS_MAX
} upgrade_status_t;

#endif
