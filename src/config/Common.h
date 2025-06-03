/*************************** Common Config page *******************************
This file is part of the pdi stack.

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
 * @define default username/ssid and password
 */
#if defined(ENABLE_HTTP_SERVER) || defined(ENABLE_AUTH_SERVICE) || defined(ENABLE_WIFI_SERVICE)
#define USER            "pdiStack"
#define PASSPHRASE      "pdiStack@123"
#endif

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


#endif
