/****************************** Server Routes *********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The `Routes.h` file defines the HTTP routes used by the web server. These routes
are mapped to specific functionalities or services, such as login, WiFi
configuration, OTA updates, GPIO management, MQTT configuration, and more.
Each route corresponds to a specific endpoint that the server handles.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _WEB_SERVER_ROUTES_
#define _WEB_SERVER_ROUTES_

// General routes
#define WEB_SERVER_HOME_ROUTE "/"                          ///< Route for the home page.
#define WEB_SERVER_LOGIN_ROUTE "/login"                   ///< Route for user login.
#define WEB_SERVER_LOGOUT_ROUTE "/logout"                 ///< Route for user logout.
#define WEB_SERVER_WIFI_CONFIG_ROUTE "/wifi-config"       ///< Route for WiFi configuration.
#define WEB_SERVER_LOGIN_CONFIG_ROUTE "/login-config"     ///< Route for login configuration.
#define WEB_SERVER_OTA_CONFIG_ROUTE "/ota-config"         ///< Route for OTA update configuration.
#define WEB_SERVER_EMAIL_CONFIG_ROUTE "/email-config"     ///< Route for email configuration.
#define WEB_SERVER_DASHBOARD_ROUTE "/dashboard"           ///< Route for the dashboard.
#define WEB_SERVER_DASHBOARD_MONITOR_ROUTE "/listen-dashboard" ///< Route for dashboard monitoring.

// GPIO-related routes
#define WEB_SERVER_GPIO_MANAGE_CONFIG_ROUTE "/gpio-manage" ///< Route for GPIO management.
#define WEB_SERVER_GPIO_SERVER_CONFIG_ROUTE "/gpio-server" ///< Route for GPIO server configuration.
#define WEB_SERVER_GPIO_MODE_CONFIG_ROUTE "/gpio-config"   ///< Route for GPIO mode configuration.
#define WEB_SERVER_GPIO_WRITE_CONFIG_ROUTE "/gpio-write"   ///< Route for GPIO write operations.
#define WEB_SERVER_GPIO_EVENT_CONFIG_ROUTE "/gpio-event"   ///< Route for GPIO event configuration.
#define WEB_SERVER_GPIO_MONITOR_ROUTE "/gpio-monitor"      ///< Route for GPIO monitoring.
#define WEB_SERVER_GPIO_ANALOG_MONITOR_ROUTE "/listen-monitor" ///< Route for analog GPIO monitoring.

// MQTT-related routes
#define WEB_SERVER_MQTT_MANAGE_CONFIG_ROUTE "/mqtt-manage" ///< Route for managing MQTT configuration.
#define WEB_SERVER_MQTT_GENERAL_CONFIG_ROUTE "/mqtt-general-config" ///< Route for general MQTT configuration.
#define WEB_SERVER_MQTT_LWT_CONFIG_ROUTE "/mqtt-lwt-config" ///< Route for MQTT Last Will and Testament (LWT) configuration.
#define WEB_SERVER_MQTT_PUBSUB_CONFIG_ROUTE "/mqtt-pubsub-config" ///< Route for MQTT publish/subscribe configuration.

// IoT device-related routes
#define WEB_SERVER_DEVICE_REGISTER_CONFIG_ROUTE "/device-register-config" ///< Route for IoT device registration.

// Storage related routes
#define WEB_SERVER_STORAGE_LIST_ROUTE "/storage" ///< Route for storage list page.
#define WEB_SERVER_STORAGE_FILE_UPLOAD_ROUTE "/storage-fileupload" ///< Route for storage file upload.
#define WEB_SERVER_STORAGE_FILE_LIST_ROUTE "/storage-filelist" ///< Route for get storage file list.
#define WEB_SERVER_STORAGE_FILE_DELETE_ROUTE "/storage-filedel" ///< Route for delete storage file.

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
