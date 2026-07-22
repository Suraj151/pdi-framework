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
#include <service_provider/session/SessionManager.h>

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
#define CMD_NAME_WHOAMI 			"whoami"
#define CMD_NAME_ID 				"id"
#define CMD_NAME_WHO 				"who"
#define CMD_NAME_SU 				"su"
#define CMD_NAME_PASSWD 			"passwd"
#define CMD_NAME_USERADD 			"useradd"
#define CMD_NAME_USERDEL 			"userdel"
#define CMD_NAME_GROUPS 			"groups"
#endif
#define CMD_NAME_GPIO 				"gpio"
#define CMD_NAME_SERVICE 			"srvc"
#define CMD_NAME_LS 				"ls"
#define CMD_NAME_CD 				"cd"
#define CMD_NAME_PWD 				"pwd"
#define CMD_NAME_MKDIR 				"mkdir"
#define CMD_NAME_TOUCH 				"touch"
#define CMD_NAME_CHMOD 				"chmod"
#define CMD_NAME_CHOWN 				"chown"
#define CMD_NAME_UMASK 				"umask"
#define CMD_NAME_RM 				"rm"
#define CMD_NAME_MOVE 			    "mv"
#define CMD_NAME_COPY 			    "cp"
#define CMD_NAME_FILE_READ 			"cat"
#define CMD_NAME_FILE_WRITE 		"fwrite"
#define CMD_NAME_CLS 			    "cls"
#define CMD_NAME_PS  			    "ps"
#define CMD_NAME_TOP  			    "top"
#define CMD_NAME_KILL 			    "kill"
#define CMD_NAME_PKILL 			    "pkill"
#define CMD_NAME_KILLALL 		    "killall"
#define CMD_NAME_RENICE 		    "renice"
#define CMD_NAME_SSH 				"ssh"
#define CMD_NAME_TLS 				"tls"
#define CMD_NAME_REBOOT				"reboot"
#define CMD_NAME_NETWORK			"net"
#define CMD_NAME_WATCH  			"watch"
#define CMD_NAME_IOT  			    "iot"
#define CMD_NAME_HELP  			    "help"
#define CMD_NAME_UPTIME  		    "uptime"
#define CMD_NAME_HEXDUMP  		    "hexdump"
#define CMD_NAME_DF  			    "df"
#define CMD_NAME_MOUNT  		    "mount"
#define CMD_NAME_WC  			    "wc"
#define CMD_NAME_HEAD  			    "head"
#define CMD_NAME_TAIL  			    "tail"
#define CMD_NAME_GREP  			    "grep"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * Resolve a command's path argument against the session PWD. Absolute args
 * (leading '/') are taken as-is; relative args are joined with PWD.
 */
static inline pdiutil::string resolveArgPath(const CommandBase::CommandOption *opt){
	pdiutil::string path;
	// optionvalsize is int16_t and the parser leaves -1 for an absent
	// positional arg — reject anything non-positive, not just zero.
	if( nullptr == opt || nullptr == opt->optionval || 0 >= opt->optionvalsize ){
		return path;
	}
	if( opt->optionval[0] == FILE_SEPARATOR[0] ){
		path.append(opt->optionval, (pdiutil::string::size_type)opt->optionvalsize);
	}else{
		path = SessionManager::getPWD();
		__i_fs.appendFileSeparator(path);
		path.append(opt->optionval, (pdiutil::string::size_type)opt->optionvalsize);
	}
	return path;
}
#endif

/* command options */
#define CMD_OPTION_NAME_A			"a"
#define CMD_OPTION_NAME_P			"p"
#define CMD_OPTION_NAME_M		    "m"
#define CMD_OPTION_NAME_N		    "n"
#define CMD_OPTION_NAME_V		    "v"
#define CMD_OPTION_NAME_U	        "u"
#define CMD_OPTION_NAME_S	        "s"
#define CMD_OPTION_NAME_Q	        "q"
#define CMD_OPTION_NAME_F	        "f"
#define CMD_OPTION_NAME_G	        "g"
#define CMD_OPTION_NAME_L	        "l"
#define CMD_OPTION_NAME_T	        "t"
#define CMD_OPTION_NAME_C	        "c"
#define CMD_OPTION_NAME_I	        "i"


#endif
