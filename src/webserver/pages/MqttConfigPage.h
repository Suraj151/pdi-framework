/*************************** MQTT Config HTML Page ****************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `MqttConfigPage.h` file defines the HTML content for various MQTT-related
configuration pages of the web server. These pages allow users to configure
general MQTT settings, Last Will and Testament (LWT) settings, and publish/subscribe
(PUBSUB) settings. The HTML content is stored in program memory (PROG_RODT_ATTR) to
optimize memory usage on embedded systems.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_MQTT_CONFIG_PAGE_H_
#define _EW_MQTT_CONFIG_PAGE_H_

#include <interface/pdi.h>

/**
 * @brief Titles for MQTT menu options.
 *
 * These strings represent the titles for different MQTT-related menu options
 * displayed on the web server.
 */
static const char WEB_SERVER_MQTT_MENU_TITLE_GENERAL[] PROG_RODT_ATTR = "mqtt general";
static const char WEB_SERVER_MQTT_MENU_TITLE_LWT[] PROG_RODT_ATTR = "mqtt lwt";
static const char WEB_SERVER_MQTT_MENU_TITLE_PUBSUB[] PROG_RODT_ATTR = "mqtt pubsub";

/**
 * @brief HTML content for the MQTT general configuration page.
 *
 * This static HTML content is used to render the top section of the MQTT general
 * configuration page on the web server. It includes a form for submitting general
 * MQTT settings.
 */
static const char WEB_SERVER_MQTT_GENERAL_PAGE_TOP[] PROG_RODT_ATTR = "\
<h2>MQTT General Configuration</h2>\
<form action='/mqtt-general-config' method='POST'>\
<table>";

/**
 * @brief HTML content for the MQTT LWT configuration page.
 *
 * This static HTML content is used to render the top section of the MQTT Last
 * Will and Testament (LWT) configuration page on the web server. It includes a
 * form for submitting LWT settings.
 */
static const char WEB_SERVER_MQTT_LWT_PAGE_TOP[] PROG_RODT_ATTR = "\
<h2>MQTT LWT Configuration</h2>\
<form action='/mqtt-lwt-config' method='POST'>\
<table>";

/**
 * @brief HTML content for the MQTT PUBSUB configuration page.
 *
 * This static HTML content is used to render the top section of the MQTT publish/
 * subscribe (PUBSUB) configuration page on the web server. It includes a form
 * for submitting PUBSUB settings.
 */
static const char WEB_SERVER_MQTT_PUBSUB_PAGE_TOP[] PROG_RODT_ATTR = "\
<h2>MQTT PUBSUB Configuration</h2>\
<form action='/mqtt-pubsub-config' method='POST'>\
<table>";

#endif
