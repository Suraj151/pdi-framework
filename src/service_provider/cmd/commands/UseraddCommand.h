/******************************** Useradd Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _USERADD_COMMAND_H_
#define _USERADD_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
#include <service_provider/user/UserStoreService.h>

struct UseraddCommand : public CommandBase {

	UseraddCommand(){
		Clear();
		SetCommand(CMD_NAME_USERADD);
		AddOption(CMD_OPTION_NAME_U);
		AddOption(CMD_OPTION_NAME_P);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_USERADD, [](void *arg)->void *{
			return new UseraddCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("useradd u=<user> p=<pass>  root-only; create a new user");
	}

	bool needauth() override { return true; }
	bool wantsMaskedInput() override { return isWaitingForOption(CMD_OPTION_NAME_P); }

	cmd_result_t execute(cmd_term_inseq_t terminputaction){

		if( needauth() && !__auth_service.getAuthorized() ){
			return CMD_RESULT_NEED_AUTH;
		}

		if( terminputaction == CMD_TERM_INSEQ_CTRL_C ||
		    terminputaction == CMD_TERM_INSEQ_CTRL_Z ){
			return CMD_RESULT_ABORTED;
		}

		const char *me = __auth_service.getUsername();
		if( nullptr == me || 0 == me[0] ) return CMD_RESULT_NEED_AUTH;
		user_record_t meRec;
		if( !__user_store_service.findUserByName(me, meRec) || meRec.m_uid != USER_STORE_ROOT_UID ){
			if( nullptr != m_terminal ) m_terminal->writeln_ro(RODT_ATTR("\nroot required"));
			return CMD_RESULT_FAILED;
		}

		CommandOption *uOpt = RetrieveOption(CMD_OPTION_NAME_U);
		CommandOption *pOpt = RetrieveOption(CMD_OPTION_NAME_P);

		bool hasU = ( nullptr != uOpt && nullptr != uOpt->optionval && uOpt->optionvalsize );
		bool hasP = ( nullptr != pOpt && nullptr != pOpt->optionval && pOpt->optionvalsize );

		if( !hasU ){
			setWaitingForOption(CMD_OPTION_NAME_U);
			if( nullptr != m_terminal ) m_terminal->write_ro(RODT_ATTR("\nuser: "));
			return CMD_RESULT_INCOMPLETE;
		}

		char uname[USER_STORE_MAX_USERNAME_LEN];
		memset(uname, 0, USER_STORE_MAX_USERNAME_LEN);
		uint16_t ulen = uOpt->optionvalsize < USER_STORE_MAX_USERNAME_LEN - 1 ? uOpt->optionvalsize : USER_STORE_MAX_USERNAME_LEN - 1;
		memcpy(uname, uOpt->optionval, ulen);

		user_record_t existing;
		if( __user_store_service.findUserByName(uname, existing) ){
			if( nullptr != m_terminal ) m_terminal->writeln_ro(RODT_ATTR("\nuser exists"));
			return CMD_RESULT_FAILED;
		}

		holdOptionValue(CMD_OPTION_NAME_U);

		if( !hasP ){
			setWaitingForOption(CMD_OPTION_NAME_P);
			if( nullptr != m_terminal ) m_terminal->write_ro(RODT_ATTR("\nPass : "));
			return CMD_RESULT_INCOMPLETE;
		}

		char pw[LOGIN_CONFIGS_BUF_SIZE];
		memset(pw, 0, LOGIN_CONFIGS_BUF_SIZE);
		uint16_t plen = pOpt->optionvalsize < LOGIN_CONFIGS_BUF_SIZE - 1 ? pOpt->optionvalsize : LOGIN_CONFIGS_BUF_SIZE - 1;
		memcpy(pw, pOpt->optionval, plen);

		uint16_t nextUid = USER_STORE_ROOT_UID + 1;
		user_record_t probe;
		while( __user_store_service.findUserByUid(nextUid, probe) ) nextUid++;

		user_record_t newRec;
		newRec.m_uid = nextUid;
		newRec.m_gid = nextUid;
		newRec.m_username = uname;
		newRec.m_home = FILE_SEPARATOR;
		newRec.m_shell = USER_STORE_DEFAULT_SHELL;

		if( __user_store_service.addUser(newRec, pw) ){
			if( nullptr != m_terminal ){
				m_terminal->write_ro(RODT_ATTR("\nadded "));
				m_terminal->write(uname);
			}
			return CMD_RESULT_OK;
		}

		return CMD_RESULT_FAILED;
	}
};

#endif

#endif
