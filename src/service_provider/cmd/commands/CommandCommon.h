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
#include <service_provider/session/Environment.h>
#ifdef ENABLE_SCRIPT_RUNNER
#include <service_provider/cmd/ScriptRunner.h>
#endif

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
#define CMD_NAME_SERVICE 			"service"
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
#define CMD_NAME_FILE_EDIT 		"fedit"
#define CMD_NAME_CLS 			    "cls"
#define CMD_NAME_PS  			    "ps"
#define CMD_NAME_TOP  			    "top"
#define CMD_NAME_KILL 			    "kill"
#define CMD_NAME_PKILL 			    "pkill"
#define CMD_NAME_KILLALL 		    "killall"
#define CMD_NAME_RENICE 		    "renice"
#define CMD_NAME_SSHKEYGEN 			"sshkgen"
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
#define CMD_NAME_ECHO  			    "echo"
#define CMD_NAME_DATE  			    "date"
#define CMD_NAME_TIMEDATECTL	    "tdctl"
#define CMD_NAME_HOST			    "host"
#define CMD_NAME_PING			    "ping"
#define CMD_NAME_DB				    "db"
#define CMD_NAME_EXEC			    "exec"
#define CMD_NAME_ENV			    "env"
#define CMD_NAME_EXPORT			    "export"
#define CMD_NAME_READ			    "read"
#define CMD_NAME_UNSET			    "unset"
#define CMD_NAME_SOURCE			    "source"
#define CMD_NAME_TEST			    "test"
#define CMD_NAME_CRONTAB		    "crontab"
#define CMD_NAME_WGET			    "wget"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * Whether something has claimed the session input descriptor, meaning a
 * command given no file still has somewhere to read from.
 */
static inline bool isInputRedirected(){
	session_t *s = SessionManager::current();
	return ( nullptr != s && nullptr != s->m_fdtable &&
			 nullptr != s->m_fdtable->m_fds[PDI_FD_STDIN] );
}

/**
 * Feeds a command its input in blocks, from the named file when one was given
 * and from the input descriptor otherwise.
 */
static inline int readCommandInput(const pdiutil::string &path, iTerminalInterface *term,
								   pdiutil::function<bool(char*, uint32_t)> sink,
								   uint16_t blocksize = 250){

	if( !path.empty() ){
		return __i_fs.readFile(path.c_str(), blocksize, sink);
	}

	if( nullptr == term ){
		return -1;
	}

	char block[64];
	int total = 0;

	while( term->available() > 0 ){

		int32_t got = term->read((uint8_t*)block, sizeof(block));
		if( got <= 0 ){
			break;
		}

		total += (int)got;
		if( !sink(block, (uint32_t)got) ){
			break;
		}
	}

	return total;
}

/**
 * Feeds a command its input one line at a time, from the named file when one
 * was given and from the input descriptor otherwise.
 */
static inline int readCommandLines(const pdiutil::string &path, iTerminalInterface *term,
								   pdiutil::function<bool(const pdiutil::string&)> online){

	pdiutil::string line;
	bool wanted = true;

	int total = readCommandInput(path, term, [&](char *data, uint32_t size)->bool{

		for( uint32_t i = 0; i < size && wanted; i++ ){

			if( '\n' != data[i] ){
				line += data[i];
				continue;
			}

			if( !line.empty() && '\r' == line.back() ){
				line.pop_back();
			}

			wanted = online(line);
			line.clear();
		}

		return wanted;
	});

	// a stream that ended without a terminator still holds one whole line
	if( total >= 0 && wanted && !line.empty() ){
		online(line);
	}

	return total;
}

/**
 * Resolve a raw path argument against the session PWD. Absolute args
 * (leading '/') are taken as-is; relative args are joined with PWD.
 */
static inline pdiutil::string resolveArgPathStr(const char *val, int16_t len){
	pdiutil::string path;
	// len is the caller-measured span; reject anything non-positive (the
	// parser leaves -1 for an absent positional arg).
	if( nullptr == val || 0 >= len ){
		return path;
	}
	if( val[0] == FILE_SEPARATOR[0] ){
		path.append(val, (pdiutil::string::size_type)len);
	}else{
		path = SessionManager::getPWD();
		__i_fs.appendFileSeparator(path);
		path.append(val, (pdiutil::string::size_type)len);
	}
	return path;
}
/**
 * Resolve a command's path argument against the session PWD.
 */
static inline pdiutil::string resolveArgPath(const CommandBase::CommandOption *opt){
	if( nullptr == opt ){
		return pdiutil::string();
	}
	return resolveArgPathStr(opt->optionval, opt->optionvalsize);
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
