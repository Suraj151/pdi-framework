/******************************** Su Command **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _SU_COMMAND_H_
#define _SU_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_AUTH_SERVICE
#ifdef ENABLE_STORAGE_SERVICE
#include <service_provider/user/UserStoreService.h>
#endif

struct SuCommand : public CommandBase {

	SuCommand(){
		Clear();
		SetCommand(CMD_NAME_SU);
		AddOption(CMD_OPTION_NAME_U);
		AddOption(CMD_OPTION_NAME_P);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_SU, [](void *arg)->void *{
			return new SuCommand();
		});
	}

	bool needauth() override { return true; }
	bool wantsMaskedInput() override { return isWaitingForOption(CMD_OPTION_NAME_P); }

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}

		char _username[LOGIN_CONFIGS_BUF_SIZE];
		char _password[LOGIN_CONFIGS_BUF_SIZE];
		memset(_username, 0, LOGIN_CONFIGS_BUF_SIZE);
		memset(_password, 0, LOGIN_CONFIGS_BUF_SIZE);

		CommandOption *unameopt = RetrieveOption(CMD_OPTION_NAME_U);
		CommandOption *passopt  = RetrieveOption(CMD_OPTION_NAME_P);

		bool isU = ( nullptr != unameopt && nullptr != unameopt->optionval && unameopt->optionvalsize );
		bool isP = ( nullptr != passopt && nullptr != passopt->optionval && passopt->optionvalsize );

		if( isU ){
			memcpy(_username, unameopt->optionval, unameopt->optionvalsize);
			if( !isP ){
				holdOptionValue(CMD_OPTION_NAME_U);
			}
		}else{
			setWaitingForOption(CMD_OPTION_NAME_U);
			if( nullptr != m_terminal ){
				m_terminal->write_ro(RODT_ATTR("\nuser: "));
			}
			return CMD_RESULT_INCOMPLETE;
		}

		if( isP ){
			memcpy(_password, passopt->optionval, passopt->optionvalsize);
		}else{
			if( terminputaction == CMD_TERM_INSEQ_CTRL_C ||
			    terminputaction == CMD_TERM_INSEQ_CTRL_Z ){
				setWaitingForOption(CMD_OPTION_NAME_U);
				return CMD_RESULT_ABORTED;
			}
			setWaitingForOption(CMD_OPTION_NAME_P);
			if( nullptr != m_terminal ){
				m_terminal->write_ro(RODT_ATTR("\nPass : "));
			}
			return CMD_RESULT_INCOMPLETE;
		}

		if( 0 == strlen(_username) || 0 == strlen(_password) ){
			return CMD_RESULT_WRONG_CREDENTIAL;
		}

		if( !__auth_service.isAuthorized(_username, _password) ){
			return CMD_RESULT_WRONG_CREDENTIAL;
		}

		__auth_service.setAuthorized(true);

#ifdef ENABLE_STORAGE_SERVICE
		user_record_t rec;
		if( __user_store_service.findUserByName(_username, rec) && !rec.m_home.empty() ){
			SessionManager::setPWD(rec.m_home.c_str());
		}
#endif

		return CMD_RESULT_OK;
	}
};

#endif

#endif
