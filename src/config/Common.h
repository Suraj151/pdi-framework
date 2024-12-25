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
#include "../../devices/DeviceConfig.h"

/**
 * @define common time durations
 */
#define MILLISECOND_DURATION_1000   1000
#define MILLISECOND_DURATION_5000   5000
#define MILLISECOND_DURATION_10000  10000

/**
 * @define default username/ssid and password
 */
#define USER            "pdiStack"
#define PASSPHRASE      "pdiStack@123"

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

/**
 * enable/disable auto factory reset on invalid database config found
 */
#define AUTO_FACTORY_RESET_ON_INVALID_CONFIGS

/**
 * enable/disable config clear/reset on factory reset event
 */
#define CONFIG_CLEAR_TO_DEFAULT_ON_FACTORY_RESET

#endif
