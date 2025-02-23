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
	cmd_result_t execute(){

		cmd_result_t result = CMD_RESULT_OK;

		if( !__auth_service.getAuthorized() ){

			char _username[LOGIN_CONFIGS_BUF_SIZE]; 
			char _password[LOGIN_CONFIGS_BUF_SIZE]; 

			memset(_username, 0, LOGIN_CONFIGS_BUF_SIZE);
			memset(_password, 0, LOGIN_CONFIGS_BUF_SIZE);

			CommandOption *usernamecmdoptn = RetrieveOption(CMD_OPTION_NAME_U);	
			CommandOption *passwordcmdoptn = RetrieveOption(CMD_OPTION_NAME_P);	

			bool isUsernameProvided = ( nullptr != usernamecmdoptn && nullptr != usernamecmdoptn->optionval && usernamecmdoptn->optionvalsize );
			bool isPasswordProvided = ( nullptr != passwordcmdoptn && nullptr != passwordcmdoptn->optionval && passwordcmdoptn->optionvalsize );

			if( isUsernameProvided ){
				memcpy(_username, usernamecmdoptn->optionval, usernamecmdoptn->optionvalsize);
				if( !isPasswordProvided ){
					holdOptionValue(CMD_OPTION_NAME_U);
				}
			}else{
				setWaitingForOption(CMD_OPTION_NAME_U);
				if( nullptr != m_terminal ){
					m_terminal->write_ro(RODT_ATTR("\n"));
      				m_terminal->write(CMD_NAME_LOGIN);
					m_terminal->write_ro(RODT_ATTR(": "));
				}
				return CMD_RESULT_INCOMPLETE;
			}

			if( isPasswordProvided ){
				memcpy(_password, passwordcmdoptn->optionval, passwordcmdoptn->optionvalsize);
			}else{
				setWaitingForOption(CMD_OPTION_NAME_P);
				if( nullptr != m_terminal ){
					m_terminal->write_ro(RODT_ATTR("\nPass : "));
				}
				return CMD_RESULT_INCOMPLETE;
			}

			if( strlen(_username) && strlen(_password) && __auth_service.isAuthorized(_username, _password) ){
				__auth_service.setAuthorized(true);
			}else{

				result = CMD_RESULT_WRONG_CREDENTIAL;
				ResultToTerminal(result);
				setWaitingForOption(CMD_OPTION_NAME_U);
				if( nullptr != m_terminal ){
					m_terminal->write_ro(RODT_ATTR("\n"));
      				m_terminal->write(CMD_NAME_LOGIN);
					m_terminal->write_ro(RODT_ATTR(": "));
				}
			}
		}

		return result;
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
	cmd_result_t execute(){

		cmd_result_t result = CMD_RESULT_OK;
		__auth_service.setAuthorized(false);
		return result;
	}
};

#endif


#endif
