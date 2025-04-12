/*************************** WiFi Config HTML Page ****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `WiFiConfigPage.h` file defines the HTML content for the WiFi configuration
page of the web server. This page allows users to configure WiFi settings, such
as SSID and password, through a web interface. The HTML content is stored in
program memory (PROGMEM) to optimize memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_WIFI_CONFIG_PAGE_H_
#define _WEB_SERVER_WIFI_CONFIG_PAGE_H_

#include <Arduino.h>

/**
 * @brief HTML content for the top section of the WiFi configuration page.
 *
 * This static HTML content is used to render the top section of the WiFi
 * configuration page on the web server. It includes a form for submitting WiFi
 * settings.
 */
static const char WEB_SERVER_WIFI_CONFIG_PAGE_TOP[] PROGMEM = "\
<h2>WiFi Configuration</h2>\
<form action='/wifi-config' method='POST'>\
<table>";

/**
 * @brief HTML content for the bottom section of the WiFi configuration page.
 *
 * This static HTML content is used to render the bottom section of the WiFi
 * configuration page on the web server. It includes a submit button for saving
 * the configuration and a button to navigate back to the home page.
 */
static const char WEB_SERVER_WIFI_CONFIG_PAGE_BOTTOM[] PROGMEM = "\
<tr>\
<td></td>\
<td>\
<button class='btn' type='submit'>\
Submit\
</button>\
<a href='/'>\
<button class='btn' type='button'>\
Home\
</button>\
</a>\
</td>\
</tr>\
</table>\
</form>\
";

#endif
