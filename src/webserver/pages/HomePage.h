/****************************** Home HTML Page ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `HomePage.h` file defines the HTML content for the home page of the web
server. The home page serves as the main entry point for users, providing
navigation options to various settings and features such as login, WiFi
configuration, OTA updates, MQTT settings, GPIO control, email settings,
device registration, and the dashboard. The HTML content is stored in program
memory (PROG_RODT_ATTR) to optimize memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_HOME_PAGE_H_
#define _WEB_SERVER_HOME_PAGE_H_

#include <interface/pdi.h>

/**
 * @brief Titles for the home page menu options.
 *
 * These strings represent the titles for different menu options displayed on
 * the home page of the web server.
 */
static const char WEB_SERVER_HOME_MENU_TITLE_LOGIN[] PROG_RODT_ATTR = "login credential";
static const char WEB_SERVER_HOME_MENU_TITLE_WIFI[] PROG_RODT_ATTR = "wifi settings";
static const char WEB_SERVER_HOME_MENU_TITLE_OTA[] PROG_RODT_ATTR = "ota update";
#ifdef ENABLE_MQTT_SERVICE
static const char WEB_SERVER_HOME_MENU_TITLE_MQTT[] PROG_RODT_ATTR = "mqtt settings";
#endif
#ifdef ENABLE_GPIO_SERVICE
static const char WEB_SERVER_HOME_MENU_TITLE_GPIO[] PROG_RODT_ATTR = "gpio control";
#endif
#ifdef ENABLE_EMAIL_SERVICE
static const char WEB_SERVER_HOME_MENU_TITLE_EMAIL[] PROG_RODT_ATTR = "email settings";
#endif
#ifdef ENABLE_DEVICE_IOT
static const char WEB_SERVER_HOME_MENU_TITLE_DEVICE_REGISTER[] PROG_RODT_ATTR = "register device";
#endif
static const char WEB_SERVER_HOME_MENU_TITLE_DASHBOARD[] PROG_RODT_ATTR = "dashboard";
static const char WEB_SERVER_HOME_MENU_TITLE_LOGOUT[] PROG_RODT_ATTR = "logout";

/**
 * @brief HTML content for the home page.
 *
 * This static HTML content is used to render the home page of the web server.
 * It includes a message prompting the user to log in and provides a button
 * linking to the login page.
 */
static const char WEB_SERVER_HOME_PAGE[] PROG_RODT_ATTR = "\
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
