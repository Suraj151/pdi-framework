/**************************** OTA Config HTML Page ****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `OtaConfigPage.h` file defines the HTML content for the OTA (Over-The-Air)
configuration page of the web server. This page allows users to configure OTA
update settings through a web interface. The HTML content is stored in program
memory (PROGMEM) to optimize memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_OTA_CONFIG_PAGE_H_
#define _WEB_SERVER_OTA_CONFIG_PAGE_H_

#include <Arduino.h>

/**
 * @brief HTML content for the OTA configuration page.
 *
 * This static HTML content is used to render the top section of the OTA
 * configuration page on the web server. It includes a form for submitting OTA
 * update settings.
 */
static const char WEB_SERVER_OTA_CONFIG_PAGE_TOP[] PROGMEM = "\
<h2>OTA Configuration</h2>\
<form action='/ota-config' method='POST'>\
<table>";

#endif
