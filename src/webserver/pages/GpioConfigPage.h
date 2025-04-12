/***************************** GPIO HTML Page *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `GpioConfigPage.h` file defines the HTML content for various GPIO-related
pages of the web server. These pages allow users to configure GPIO modes, control
GPIO states, monitor GPIO activity, and manage GPIO alerts. The HTML content is
stored in program memory (PROGMEM) to optimize memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_GPIO_CONFIG_PAGE_H_
#define _EW_GPIO_CONFIG_PAGE_H_

#include <Arduino.h>

/**
 * @brief Titles for GPIO menu options.
 *
 * These strings represent the titles for different GPIO-related menu options
 * displayed on the web server.
 */
static const char WEB_SERVER_GPIO_MENU_TITLE_MODES[] PROGMEM = "gpio modes";
static const char WEB_SERVER_GPIO_MENU_TITLE_CONTROL[] PROGMEM = "gpio control";
static const char WEB_SERVER_GPIO_MENU_TITLE_SERVER[] PROGMEM = "gpio server";
static const char WEB_SERVER_GPIO_MENU_TITLE_MONITOR[] PROGMEM = "gpio monitor";
static const char WEB_SERVER_GPIO_MENU_TITLE_ALERT[] PROGMEM = "gpio alerts";

/**
 * @brief HTML content for the GPIO monitor page.
 *
 * This static HTML content is used to render the top section of the GPIO monitor
 * page on the web server.
 */
static const char WEB_SERVER_GPIO_MONITOR_PAGE_TOP[] PROGMEM = "\
<h2>GPIO Monitor</h2>\
";

/**
 * @brief SVG element for GPIO monitoring.
 *
 * This SVG element is used to render a graphical representation of GPIO activity
 * on the GPIO monitor page.
 */
static const char WEB_SERVER_GPIO_MONITOR_SVG_ELEMENT[] PROGMEM = "\
<svg id='svga0' width='300' height='150'>\
<rect width='300' height='150'/>\
<line x1='0' y1='25' x2='300' y2='25' stroke='white'></line>\
<line x1='0' y1='125' x2='300' y2='125' stroke='white'></line>\
</svg>\
";

/**
 * @brief HTML content for the GPIO server configuration page.
 *
 * This static HTML content is used to render the top section of the GPIO server
 * configuration page on the web server.
 */
static const char WEB_SERVER_GPIO_SERVER_PAGE_TOP[] PROGMEM = "\
<h2>GPIO Server Configuration</h2>\
<form action='/gpio-server' method='POST'>\
<table>";

/**
 * @brief HTML content for the GPIO mode configuration page.
 *
 * This static HTML content is used to render the top section of the GPIO mode
 * configuration page on the web server.
 */
static const char WEB_SERVER_GPIO_CONFIG_PAGE_TOP[] PROGMEM = "\
<h2>GPIO Mode Configuration</h2>\
<form action='/gpio-config' method='POST'>\
<table>";

/**
 * @brief HTML content for the GPIO control page.
 *
 * This static HTML content is used to render the top section of the GPIO control
 * page on the web server.
 */
static const char WEB_SERVER_GPIO_WRITE_PAGE_TOP[] PROGMEM = "\
<h2>GPIO Control</h2>\
<form action='/gpio-write' method='POST'>\
<table>";

/**
 * @brief HTML content for the GPIO alert control page.
 *
 * This static HTML content is used to render the top section of the GPIO alert
 * control page on the web server.
 */
static const char WEB_SERVER_GPIO_ALERT_PAGE_TOP[] PROGMEM = "\
<h2>GPIO Alert Control</h2>\
<form action='/gpio-alert' method='POST'>\
<table>";

/**
 * @brief HTML message for empty GPIO alert configuration.
 *
 * This static HTML content is displayed when no GPIO is enabled for alert
 * operations. It provides a link to enable GPIO modes.
 */
static const char WEB_SERVER_GPIO_ALERT_EMPTY_MESSAGE[] PROGMEM = "\
<h4>No GPIO enabled for operation.</h4>\
<div>enable from \
<a href='/gpio-config'>\
<button class='btn' type='button'>\
GPIO Modes\
</button>\
</a>\
</div>\
</table>\
</form>\
";

/**
 * @brief HTML message for empty GPIO write configuration.
 *
 * This message reuses the `WEB_SERVER_GPIO_ALERT_EMPTY_MESSAGE` to indicate
 * that no GPIO is enabled for write operations.
 */
static const char *WEB_SERVER_GPIO_WRITE_EMPTY_MESSAGE = WEB_SERVER_GPIO_ALERT_EMPTY_MESSAGE;

#endif
