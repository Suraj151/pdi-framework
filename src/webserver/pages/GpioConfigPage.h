/***************************** gpio html page *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_GPIO_CONFIG_PAGE_H_
#define _EW_GPIO_CONFIG_PAGE_H_

#include <Arduino.h>

static const char EW_SERVER_GPIO_MENU_TITLE_MODES     [] PROGMEM = "gpio modes";
static const char EW_SERVER_GPIO_MENU_TITLE_CONTROL   [] PROGMEM = "gpio control";
static const char EW_SERVER_GPIO_MENU_TITLE_SERVER    [] PROGMEM = "gpio server";
static const char EW_SERVER_GPIO_MENU_TITLE_MONITOR   [] PROGMEM = "gpio monitor";
static const char EW_SERVER_GPIO_MENU_TITLE_ALERT     [] PROGMEM = "gpio alerts";

static const char EW_SERVER_GPIO_MONITOR_PAGE_TOP[] PROGMEM = "\
<h2>GPIO Monitor</h2>\
";

static const char EW_SERVER_GPIO_MONITOR_SVG_ELEMENT[] PROGMEM = "\
<svg id='svga0' width='300' height='150'>\
<rect width='300' height='150'/>\
<line x1='0' y1='25' x2='300' y2='25' stroke='white'></line>\
<line x1='0' y1='125' x2='300' y2='125' stroke='white'></line>\
</svg>\
";

static const char EW_SERVER_GPIO_SERVER_PAGE_TOP[] PROGMEM = "\
<h2>GPIO Server Configuration</h2>\
<form action='/gpio-server' method='POST'>\
<table>";

static const char EW_SERVER_GPIO_CONFIG_PAGE_TOP[] PROGMEM = "\
<h2>GPIO Mode Configuration</h2>\
<form action='/gpio-config' method='POST'>\
<table>";

static const char EW_SERVER_GPIO_WRITE_PAGE_TOP[] PROGMEM = "\
<h2>GPIO Control</h2>\
<form action='/gpio-write' method='POST'>\
<table>";

static const char EW_SERVER_GPIO_ALERT_PAGE_TOP[] PROGMEM = "\
<h2>GPIO Alert Control</h2>\
<form action='/gpio-alert' method='POST'>\
<table>";

static const char EW_SERVER_GPIO_ALERT_EMPTY_MESSAGE[] PROGMEM = "\
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

static const char* EW_SERVER_GPIO_WRITE_EMPTY_MESSAGE = EW_SERVER_GPIO_ALERT_EMPTY_MESSAGE;

// static const char EW_SERVER_GPIO_WRITE_EMPTY_MESSAGE[] PROGMEM = "\
// <h4>No GPIO enabled for write operation.</h4>\
// <div>enable from \
// <a href='/gpio-config'>\
// <button class='btn' type='button'>\
// GPIO Modes\
// </button>\
// </a>\
// </div>\
// </table>\
// </form>\
// ";

#endif
