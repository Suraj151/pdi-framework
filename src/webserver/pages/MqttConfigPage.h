/*************************** mqtt config html page ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_MQTT_CONFIG_PAGE_H_
#define _EW_MQTT_CONFIG_PAGE_H_

#include <Arduino.h>

static const char WEB_SERVER_MQTT_MENU_TITLE_GENERAL     [] PROGMEM = "mqtt general";
static const char WEB_SERVER_MQTT_MENU_TITLE_LWT         [] PROGMEM = "mqtt lwt";
static const char WEB_SERVER_MQTT_MENU_TITLE_PUBSUB      [] PROGMEM = "mqtt pubsub";

static const char WEB_SERVER_MQTT_GENERAL_PAGE_TOP[] PROGMEM = "\
<h2>MQTT General Configuration</h2>\
<form action='/mqtt-general-config' method='POST'>\
<table>";

static const char WEB_SERVER_MQTT_LWT_PAGE_TOP[] PROGMEM = "\
<h2>MQTT LWT Configuration</h2>\
<form action='/mqtt-lwt-config' method='POST'>\
<table>";

static const char WEB_SERVER_MQTT_PUBSUB_PAGE_TOP[] PROGMEM = "\
<h2>MQTT PUBSUB Configuration</h2>\
<form action='/mqtt-pubsub-config' method='POST'>\
<table>";

#endif
