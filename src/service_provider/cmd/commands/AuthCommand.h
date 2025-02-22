/********************************* Auth Command *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _AUTH_COMMAND_H_
#define _AUTH_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_AUTH_SERVICE

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
		AddOption(CMD_OPTION_NAME_U);
		AddOption(CMD_OPTION_NAME_P);
	}

	/* execute command with provided options */
	cmd_status_t execute(){

		cmd_status_t status = CMD_STATUS_OK;

		if( !__auth_service.getAuthorized() ){

			char _username[LOGIN_CONFIGS_BUF_SIZE]; 
			char _password[LOGIN_CONFIGS_BUF_SIZE]; 

			memset(_username, 0, LOGIN_CONFIGS_BUF_SIZE);
			memset(_password, 0, LOGIN_CONFIGS_BUF_SIZE);

			CommandOption *cmdoptn = RetrieveOption(CMD_OPTION_NAME_U);		
			if( nullptr != cmdoptn ){
				memcpy(_username, cmdoptn->optionval, cmdoptn->optionvalsize);
			}

			cmdoptn = RetrieveOption(CMD_OPTION_NAME_P);
			if( nullptr != cmdoptn ){
				memcpy(_password, cmdoptn->optionval, cmdoptn->optionvalsize);
			}

			if( strlen(_username) && strlen(_password) && __auth_service.isAuthorized(_username, _password) ){
				__auth_service.setAuthorized(true);
			}else{

				status = CMD_STATUS_WRONG_CREDENTIAL;
			}
		}

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
		__auth_service.setAuthorized(false);
		return status;
	}
};

#endif


#endif
