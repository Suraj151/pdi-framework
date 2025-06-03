/******************************** CMD common *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _COMMANDCOMMON_H_
#define _COMMANDCOMMON_H_

#include <config/Config.h>

#ifdef ENABLE_AUTH_SERVICE
#include <service_provider/auth/AuthServiceProvider.h>
#endif

#ifdef ENABLE_GPIO_SERVICE
#include <service_provider/device/GpioServiceProvider.h>
#endif

#ifdef ENABLE_WIFI_SERVICE
#include <service_provider/network/WiFiServiceProvider.h>
#endif

#ifdef ENABLE_OTA_SERVICE
#include <service_provider/device/OtaServiceProvider.h>
#endif

#ifdef ENABLE_MQTT_SERVICE
#include <service_provider/transport/MqttServiceProvider.h>
#endif

#ifdef ENABLE_EMAIL_SERVICE
#include <service_provider/email/EmailServiceProvider.h>
#endif

#ifdef ENABLE_DEVICE_IOT
#include <service_provider/iot/DeviceIotServiceProvider.h>
#endif


/* command lists */
#ifdef ENABLE_AUTH_SERVICE
#define CMD_NAME_LOGIN 				"login"
#define CMD_NAME_LOGOUT				"logout"
#endif
#define CMD_NAME_GPIO 				"gpio"
#define CMD_NAME_SERVICE 			"srvc"
#define CMD_NAME_LS 				"ls"
#define CMD_NAME_CD 				"cd"
#define CMD_NAME_PWD 				"pwd"
#define CMD_NAME_MKD 				"mkd"
#define CMD_NAME_MKF 				"mkf"
#define CMD_NAME_RM 				"rm"
#define CMD_NAME_MOVE 			    "mv"
#define CMD_NAME_FILE_READ 			"frd"
#define CMD_NAME_FILE_WRITE 		"fwr"
#define CMD_NAME_CLS 			    "cls"
#define CMD_NAME_SCHED_TASK 		"scht"

/* command options */
#define CMD_OPTION_NAME_P			"p"
#define CMD_OPTION_NAME_M		    "m"
#define CMD_OPTION_NAME_V		    "v"
#define CMD_OPTION_NAME_U	        "u"
#define CMD_OPTION_NAME_S	        "s"
#define CMD_OPTION_NAME_Q	        "q"
#define CMD_OPTION_NAME_F	        "f"
#define CMD_OPTION_NAME_L	        "l"


#endif
