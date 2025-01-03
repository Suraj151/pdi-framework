/******************************** CMD Service *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _COMMANDLINE_SERVICE_H_
#define _COMMANDLINE_SERVICE_H_

#include <service_provider/ServiceProvider.h>
#include "GpioServiceProvider.h"
#include "AuthServiceProvider.h"

/**
 * CommandLineServiceProvider class
 */
class CommandLineServiceProvider : public ServiceProvider
{

public:
	/**
	 * CommandLineServiceProvider constructor
	 */
	CommandLineServiceProvider();

	/**
	 * CommandLineServiceProvider destructor
	 */
	~CommandLineServiceProvider();

	cmd_status_t executeCommand(pdiutil::string *cmd = nullptr);
	static void CMDStatusToSerial(cmd_status_t status);

private:

	/**
	 * @var	pdiutil::vector<cmd_t>	m_cmdlist
	 */
    pdiutil::vector<cmd_t*> m_cmdlist;
};

extern CommandLineServiceProvider __cmd_service;


/* command lists */
#define CMD_NAME_LOGIN 				"login"
#define CMD_NAME_LOGOUT				"logout"
#define CMD_NAME_GPIO 				"gpio"

/* command options */
#define CMD_OPTION_NAME_PIN			"p"
#define CMD_OPTION_NAME_MODE		"m"
#define CMD_OPTION_NAME_VALUE		"v"
#define CMD_OPTION_NAME_USERNAME	"u"
#define CMD_OPTION_NAME_PASSWORD	CMD_OPTION_NAME_PIN

/**
 * login command
 * 
 * e.g. login u=pdiStack, p=pdiStack@123
 */
struct LoginCommand : public CommandBase {

	/* Constructor */
	LoginCommand(){
		Clear();
		SetCommand(CMD_NAME_LOGIN);
		AddOption(CMD_OPTION_NAME_USERNAME);
		AddOption(CMD_OPTION_NAME_PASSWORD);
	}

	/* execute command with provided options */
	cmd_status_t execute(){

		cmd_status_t status = CMD_STATUS_OK;
		LogFmtI("executing command : %s with", cmd);

		char _username[LOGIN_CONFIGS_BUF_SIZE]; 
		char _password[LOGIN_CONFIGS_BUF_SIZE]; 

		memset(_username, 0, LOGIN_CONFIGS_BUF_SIZE);
		memset(_password, 0, LOGIN_CONFIGS_BUF_SIZE);

		for (uint8_t i = 0; i < CMD_OPTION_MAX; i++){

			if(options[i].optionval != nullptr){

				if(__are_str_equals(options[i].option, CMD_OPTION_NAME_USERNAME, CMD_OPTION_SIZE_MAX)){

					memcpy(_username, options[i].optionval, options[i].optionvalsize);
					LogFmtI(" | option %d : %s, %s", i, options[i].option, _username);
				}else if(__are_str_equals(options[i].option, CMD_OPTION_NAME_PASSWORD, CMD_OPTION_SIZE_MAX)){

					memcpy(_password, options[i].optionval, options[i].optionvalsize);
					LogFmtI(" | option %d : %s, %s", i, options[i].option, _password);
				}
			}
		}

		if( strlen(_username) && strlen(_password) && __auth_service.isAuthorized(_username, _password) ){
			__auth_service.setAuthorized(true);
		}else{

			status = CMD_STATUS_WRONG_CREDENTIAL;
		}

		LogI("\n");
		return status;
	}
};

/**
 * logout command
 * 
 */
struct LogoutCommand : public CommandBase {

	/* Constructor */
	LogoutCommand(){
		Clear();
		SetCommand(CMD_NAME_LOGOUT);
	}

	/* execute command with provided options */
	cmd_status_t execute(){

		cmd_status_t status = CMD_STATUS_OK;
		LogFmtI("executing command : %s\n", cmd);
		__auth_service.setAuthorized(false);
		return status;
	}
};

/**
 * gpio command
 * 
 * e.g. if we want to blink 4th gpio @500ms then we can execute command as below
 * gpio p=4, m=3, v=500
 */
struct GpioCommand : public CommandBase {

	/* Constructor */
	GpioCommand(){
		Clear();
		SetCommand(CMD_NAME_GPIO);
		AddOption(CMD_OPTION_NAME_PIN);
		AddOption(CMD_OPTION_NAME_MODE);
		AddOption(CMD_OPTION_NAME_VALUE);
	}

	/* override the necesity of required permission */
	bool needauth() override { return true; }

	/* execute command with provided options */
	cmd_status_t execute(){

		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_STATUS_NEED_AUTH;
		}

		cmd_status_t status = CMD_STATUS_OK;
		LogFmtI("executing command : %s with", cmd);

		int16_t _pin = -1; 
		int16_t _mode = -1; 
		int32_t _value = -1; 
		int32_t _strval = -1;

		for (uint8_t i = 0; i < CMD_OPTION_MAX; i++){

			if(options[i].optionval != nullptr){

				// LogFmtI(" | option %d : %s, %s, %d", i, options[i].option, options[i].optionval, options[i].optionvalsize);
				_strval = StringToUint16(options[i].optionval, options[i].optionvalsize);

				if(__are_str_equals(options[i].option, CMD_OPTION_NAME_PIN, CMD_OPTION_SIZE_MAX)){

					_pin = _strval;
				}else if(__are_str_equals(options[i].option, CMD_OPTION_NAME_MODE, CMD_OPTION_SIZE_MAX)){

					_mode = _strval;
				}else if(__are_str_equals(options[i].option, CMD_OPTION_NAME_VALUE, CMD_OPTION_SIZE_MAX)){

					_value = _strval;
				}

				LogFmtI(" | option %d : %s, %d", i, options[i].option, _strval);
			}
		}

		if( _pin != -1 ){

			if( _mode != -1 ){
				
				__gpio_service.m_gpio_config_copy.gpio_mode[_pin] = _mode;
			}

			if( _value != -1 ){
				
				__gpio_service.m_gpio_config_copy.gpio_readings[_pin] = _value;
			}

			__database_service.set_gpio_config_table(&__gpio_service.m_gpio_config_copy);
			__gpio_service.handleGpioModes(GPIO_WRITE_CONFIG);
		}else{

			status = CMD_STATUS_ARGS_MISSING;
		}

		LogI("\n");
		return status;
	}
};


#endif
