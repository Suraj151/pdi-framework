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

	static constexpr int8_t SLOT_USER = 0;
	static constexpr int8_t SLOT_PASS = 1;

	SuCommand(){
		Clear();
		SetCommand(CMD_NAME_SU);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
		reservePositionalSlots(2);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_SU, [](void *arg)->void *{
			return new SuCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("su [<user> [<pass>]]  switch user in the current session");
	}

	bool needauth() override { return true; }
	bool wantsMaskedInput() override { return isWaitingForOption(SLOT_PASS); }

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}

		if( terminputaction == CMD_TERM_INSEQ_CTRL_C ||
		    terminputaction == CMD_TERM_INSEQ_CTRL_Z ){
			return CMD_RESULT_ABORTED;
		}

		char _username[LOGIN_CONFIGS_BUF_SIZE];
		char _password[LOGIN_CONFIGS_BUF_SIZE];
		memset(_username, 0, LOGIN_CONFIGS_BUF_SIZE);
		memset(_password, 0, LOGIN_CONFIGS_BUF_SIZE);

		CommandOption *unameopt = &m_options[SLOT_USER];
		CommandOption *passopt  = &m_options[SLOT_PASS];

		bool isU = ( nullptr != unameopt->optionval && unameopt->optionvalsize > 0 );
		bool isP = ( nullptr != passopt->optionval && passopt->optionvalsize > 0 );

		if( !isU ){
			setWaitingForOption(SLOT_USER);
			if( nullptr != m_terminal ){
				m_terminal->write_ro(RODT_ATTR("\nuser: "));
			}
			return CMD_RESULT_INCOMPLETE;
		}

		uint16_t ulen = unameopt->optionvalsize < LOGIN_CONFIGS_BUF_SIZE - 1 ? unameopt->optionvalsize : LOGIN_CONFIGS_BUF_SIZE - 1;
		memcpy(_username, unameopt->optionval, ulen);

		if( !isP ){
			holdOptionValue(SLOT_USER);
			setWaitingForOption(SLOT_PASS);
			if( nullptr != m_terminal ){
				m_terminal->write_ro(RODT_ATTR("\nPass : "));
			}
			return CMD_RESULT_INCOMPLETE;
		}

		uint16_t plen = passopt->optionvalsize < LOGIN_CONFIGS_BUF_SIZE - 1 ? passopt->optionvalsize : LOGIN_CONFIGS_BUF_SIZE - 1;
		memcpy(_password, passopt->optionval, plen);

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
