/****************************** server routes *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _EW_SERVER_ROUTES_
#define _EW_SERVER_ROUTES_

#define EW_SERVER_HOME_ROUTE "/"
#define EW_SERVER_LOGIN_ROUTE "/login"
#define EW_SERVER_LOGOUT_ROUTE "/logout"
#define EW_SERVER_WIFI_CONFIG_ROUTE "/wifi-config"
#define EW_SERVER_LOGIN_CONFIG_ROUTE "/login-config"
#define EW_SERVER_OTA_CONFIG_ROUTE "/ota-config"
#define EW_SERVER_EMAIL_CONFIG_ROUTE "/email-config"
#define EW_SERVER_DASHBOARD_ROUTE "/dashboard"
#define EW_SERVER_DASHBOARD_MONITOR_ROUTE "/listen-dashboard"

#define EW_SERVER_GPIO_MANAGE_CONFIG_ROUTE "/gpio-manage"
#define EW_SERVER_GPIO_SERVER_CONFIG_ROUTE "/gpio-server"
#define EW_SERVER_GPIO_MODE_CONFIG_ROUTE "/gpio-config"
#define EW_SERVER_GPIO_WRITE_CONFIG_ROUTE "/gpio-write"
#define EW_SERVER_GPIO_ALERT_CONFIG_ROUTE "/gpio-alert"
#define EW_SERVER_GPIO_MONITOR_ROUTE "/gpio-monitor"
#define EW_SERVER_GPIO_ANALOG_MONITOR_ROUTE "/listen-monitor"

#define EW_SERVER_MQTT_MANAGE_CONFIG_ROUTE "/mqtt-manage"
#define EW_SERVER_MQTT_GENERAL_CONFIG_ROUTE "/mqtt-general-config"
#define EW_SERVER_MQTT_LWT_CONFIG_ROUTE "/mqtt-lwt-config"
#define EW_SERVER_MQTT_PUBSUB_CONFIG_ROUTE "/mqtt-pubsub-config"

#define EW_SERVER_DEVICE_REGISTER_CONFIG_ROUTE "/device-register-config"

#endif

// var _sv=document.getElementById('svga0');
// var ln = document.createElementNS('http://www.w3.org/2000/svg','line');
// ln.setAttribute('x1',0);
// ln.setAttribute('y1',0);
// ln.setAttribute('x2',0);
// ln.setAttribute('y2',0);
// ln.setAttribute('stroke','red');
// if(_sv.childElementCount>10)_sv.removeChild(_sv.childNodes[0]);
// _sv.appendChild(ln)
