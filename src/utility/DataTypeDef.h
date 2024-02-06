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

typedef uint8_t   gpio_id_t;
typedef int64_t   gpio_val_t;

typedef uint8_t   wifi_mode_t;
typedef uint8_t   sleep_mode_t;
typedef uint16_t  wifi_status_t;
typedef uint8_t   follow_redirects_t;

typedef uint16_t  http_method_t;

/**
 * Define required callback type
 */
typedef std::function<void(int)> CallBackIntArgFn;
typedef std::function<void(void)> CallBackVoidArgFn;
typedef std::function<void(void*)> CallBackVoidPointerArgFn;

typedef enum {
  INFO_LOG,
  ERROR_LOG,
  WARNING_LOG,
  SUCCESS_LOG
} logger_type_t;

#endif
