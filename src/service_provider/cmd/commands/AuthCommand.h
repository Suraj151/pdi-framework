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

    /**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_LOGIN, [](void *arg)->void *{
			return pdiutil::safe_new<LoginCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("login [u=<user>,p=<pass>]  interactive login (three-line if args omitted)");
	}

	bool wantsMaskedInput() override { return isWaitingForOption(CMD_OPTION_NAME_P); }

	/* execute command with provided options */
	pdi_err_t execute(cmd_term_inseq_t terminputaction){

		pdi_err_t result = PDI_OK;

		if( !__auth_service.getAuthorized() ){

			char _username[LOGIN_CONFIGS_BUF_SIZE]; 
			char _password[LOGIN_CONFIGS_BUF_SIZE]; 

			memset(_username, 0, LOGIN_CONFIGS_BUF_SIZE);
			memset(_password, 0, LOGIN_CONFIGS_BUF_SIZE);

			CommandOption *usernamecmdoptn = RetrieveOption(CMD_OPTION_NAME_U);	
			CommandOption *passwordcmdoptn = RetrieveOption(CMD_OPTION_NAME_P);	

			bool isUsernameProvided = ( nullptr != usernamecmdoptn && nullptr != usernamecmdoptn->optionval && usernamecmdoptn->optionvalsize > 0 );
			bool isPasswordProvided = ( nullptr != passwordcmdoptn && nullptr != passwordcmdoptn->optionval && passwordcmdoptn->optionvalsize > 0 );

			if( isUsernameProvided ){
				int16_t _ulen = usernamecmdoptn->optionvalsize < (LOGIN_CONFIGS_BUF_SIZE-1) ? usernamecmdoptn->optionvalsize : (LOGIN_CONFIGS_BUF_SIZE-1);
				memcpy(_username, usernamecmdoptn->optionval, _ulen);
				if( !isPasswordProvided ){
					holdOptionValue(CMD_OPTION_NAME_U);
				}
			}else{
				setWaitingForOption(CMD_OPTION_NAME_U);
				if( nullptr != m_terminal ){
					m_terminal->putln();
      				m_terminal->write(CMD_NAME_LOGIN);
					m_terminal->write_ro(RODT_ATTR(": "));
				}
				return CMD_ERROR_AGAIN;
			}

			bool isPasswordAnswered = isWaitingForOption(CMD_OPTION_NAME_P) &&
									  terminputaction == CMD_TERM_INSEQ_ENTER;

			if( isPasswordProvided ){
				int16_t _plen = passwordcmdoptn->optionvalsize < (LOGIN_CONFIGS_BUF_SIZE-1) ? passwordcmdoptn->optionvalsize : (LOGIN_CONFIGS_BUF_SIZE-1);
				memcpy(_password, passwordcmdoptn->optionval, _plen);
			}else{

				if( terminputaction == CMD_TERM_INSEQ_CTRL_C ||
					terminputaction == CMD_TERM_INSEQ_CTRL_Z ){
					setWaitingForOption(CMD_OPTION_NAME_U);
					return CMD_ERROR_CANCELED;
				}

				if( !isPasswordAnswered ){
					setWaitingForOption(CMD_OPTION_NAME_P);
					if( nullptr != m_terminal ){
						m_terminal->putln();
						m_terminal->write_ro(RODT_ATTR("Pass : "));
					}
					return CMD_ERROR_AGAIN;
				}
			}

			if( strlen(_username) && strlen(_password) && __auth_service.isAuthorized(_username, _password) ){
				__auth_service.setAuthorized(true);
				__auth_service.enterHome();
			}else{

				result = CMD_ERROR_ACCES;
				ResultToTerminal(result);
				setWaitingForOption(CMD_OPTION_NAME_U);
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

    /**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_LOGOUT, [](void *arg)->void *{
			return pdiutil::safe_new<LogoutCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("logout  end the session (or close the telnet/SSH channel)");
	}

	/* execute command with provided options */
	pdi_err_t execute(cmd_term_inseq_t terminputaction){

		__auth_service.setAuthorized(false);
		Environment::clearSession();
		#ifdef ENABLE_SCRIPT_RUNNER
		ScriptRunner::clearSession();
		#endif

		#ifdef ENABLE_STORAGE_SERVICE
		SessionManager::setPWD(__i_fs.getHomeDirectory());
		#endif

		if( nullptr != m_terminal ){
			terminal_types_t t = m_terminal->get_terminal_type();
			if( TERMINAL_TYPE_TELNET == t || TERMINAL_TYPE_SSH == t ){
				return CMD_ERROR_INTR;
			}
		}
		return PDI_OK;
	}
};

#endif


#endif
