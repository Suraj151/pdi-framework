/**************************** Device IoT HTML Pages ***************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `DeviceIotPage.h` file defines the HTML content for the IoT device
registration configuration page of the web server. This page allows users to
configure and register IoT devices through a web interface. The HTML content
is stored in program memory (PROGMEM) to optimize memory usage on embedded
systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_DEVICE_IOT_PAGE_H_
#define _WEB_SERVER_DEVICE_IOT_PAGE_H_

#include <Arduino.h>

/**
 * @brief HTML content for the IoT device registration configuration page.
 *
 * This static HTML content is used to render the top section of the IoT device
 * registration configuration page on the web server. It includes a form for
 * submitting device registration details.
 */
static const char WEB_SERVER_DEVICE_REGISTER_CONFIG_PAGE_TOP[] PROGMEM = "\
<h2>Device Register Configuration</h2>\
<form id='drcf' action='/device-register-config' method='POST'>\
<table>";

#endif
