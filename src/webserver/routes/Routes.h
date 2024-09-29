/****************************** server routes *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_ROUTES_
#define _WEB_SERVER_ROUTES_

#define WEB_SERVER_HOME_ROUTE "/"
#define WEB_SERVER_LOGIN_ROUTE "/login"
#define WEB_SERVER_LOGOUT_ROUTE "/logout"
#define WEB_SERVER_WIFI_CONFIG_ROUTE "/wifi-config"
#define WEB_SERVER_LOGIN_CONFIG_ROUTE "/login-config"
#define WEB_SERVER_OTA_CONFIG_ROUTE "/ota-config"
#define WEB_SERVER_EMAIL_CONFIG_ROUTE "/email-config"
#define WEB_SERVER_DASHBOARD_ROUTE "/dashboard"
#define WEB_SERVER_DASHBOARD_MONITOR_ROUTE "/listen-dashboard"

#define WEB_SERVER_GPIO_MANAGE_CONFIG_ROUTE "/gpio-manage"
#define WEB_SERVER_GPIO_SERVER_CONFIG_ROUTE "/gpio-server"
#define WEB_SERVER_GPIO_MODE_CONFIG_ROUTE "/gpio-config"
#define WEB_SERVER_GPIO_WRITE_CONFIG_ROUTE "/gpio-write"
#define WEB_SERVER_GPIO_ALERT_CONFIG_ROUTE "/gpio-alert"
#define WEB_SERVER_GPIO_MONITOR_ROUTE "/gpio-monitor"
#define WEB_SERVER_GPIO_ANALOG_MONITOR_ROUTE "/listen-monitor"

#define WEB_SERVER_MQTT_MANAGE_CONFIG_ROUTE "/mqtt-manage"
#define WEB_SERVER_MQTT_GENERAL_CONFIG_ROUTE "/mqtt-general-config"
#define WEB_SERVER_MQTT_LWT_CONFIG_ROUTE "/mqtt-lwt-config"
#define WEB_SERVER_MQTT_PUBSUB_CONFIG_ROUTE "/mqtt-pubsub-config"

#define WEB_SERVER_DEVICE_REGISTER_CONFIG_ROUTE "/device-register-config"

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
