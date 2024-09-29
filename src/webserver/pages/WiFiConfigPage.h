/*************************** wifi config html page ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_SERVER_WIFI_CONFIG_PAGE_H_
#define _EW_SERVER_WIFI_CONFIG_PAGE_H_

#include <Arduino.h>

static const char EW_SERVER_WIFI_CONFIG_PAGE_TOP[] PROGMEM = "\
<h2>WiFi Configuration</h2>\
<form action='/wifi-config' method='POST'>\
<table>";

static const char EW_SERVER_WIFI_CONFIG_PAGE_BOTTOM[] PROGMEM = "\
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
