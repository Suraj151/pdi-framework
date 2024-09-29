/****************************** home html page ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_HOME_PAGE_H_
#define _WEB_SERVER_HOME_PAGE_H_

#include <Arduino.h>
#include <config/Config.h>

static const char WEB_SERVER_HOME_MENU_TITLE_LOGIN     [] PROGMEM = "login credential";
static const char WEB_SERVER_HOME_MENU_TITLE_WIFI      [] PROGMEM = "wifi settings";
static const char WEB_SERVER_HOME_MENU_TITLE_OTA       [] PROGMEM = "ota update";
#ifdef ENABLE_MQTT_SERVICE
static const char WEB_SERVER_HOME_MENU_TITLE_MQTT      [] PROGMEM = "mqtt settings";
#endif
#ifdef ENABLE_GPIO_SERVICE
static const char WEB_SERVER_HOME_MENU_TITLE_GPIO      [] PROGMEM = "gpio control";
#endif
#ifdef ENABLE_EMAIL_SERVICE
static const char WEB_SERVER_HOME_MENU_TITLE_EMAIL     [] PROGMEM = "email settings";
#endif
#ifdef ENABLE_DEVICE_IOT
static const char WEB_SERVER_HOME_MENU_TITLE_DEVICE_REGISTER  [] PROGMEM = "register device";
#endif
static const char WEB_SERVER_HOME_MENU_TITLE_DASHBOARD [] PROGMEM = "dashboard";
static const char WEB_SERVER_HOME_MENU_TITLE_LOGOUT    [] PROGMEM = "logout";

static const char WEB_SERVER_HOME_PAGE[] PROGMEM = "\
<h4>Please login to change settings</h4>\
<div>\
<a href='/login'>\
<button class='btn'>\
Click Here\
</button>\
</a>\
to login\
</div>";

#endif
