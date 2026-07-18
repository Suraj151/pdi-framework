/******************************** Userdel Command *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 18th July 2026
******************************************************************************/
#ifndef _USERDEL_COMMAND_H_
#define _USERDEL_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_AUTH_SERVICE) && defined(ENABLE_STORAGE_SERVICE)
#include <service_provider/user/UserStoreService.h>

struct UserdelCommand : public CommandBase {

	UserdelCommand(){
		Clear();
		SetCommand(CMD_NAME_USERDEL);
		AddOption(CMD_OPTION_NAME_U);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_USERDEL, [](void *arg)->void *{
			return new UserdelCommand();
		});
	}

	bool needauth() override { return true; }

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
		bool hasU = ( nullptr != uOpt && nullptr != uOpt->optionval && uOpt->optionvalsize );

		if( !hasU ){
			setWaitingForOption(CMD_OPTION_NAME_U);
			if( nullptr != m_terminal ) m_terminal->write_ro(RODT_ATTR("\nuser: "));
			return CMD_RESULT_INCOMPLETE;
		}

		char uname[USER_STORE_MAX_USERNAME_LEN];
		memset(uname, 0, USER_STORE_MAX_USERNAME_LEN);
		uint16_t ulen = uOpt->optionvalsize < USER_STORE_MAX_USERNAME_LEN - 1 ? uOpt->optionvalsize : USER_STORE_MAX_USERNAME_LEN - 1;
		memcpy(uname, uOpt->optionval, ulen);

		if( 0 == strcmp(uname, me) ){
			if( nullptr != m_terminal ) m_terminal->writeln_ro(RODT_ATTR("\ncannot delete self"));
			return CMD_RESULT_FAILED;
		}

		user_record_t target;
		if( !__user_store_service.findUserByName(uname, target) ){
			if( nullptr != m_terminal ) m_terminal->writeln_ro(RODT_ATTR("\nno such user"));
			return CMD_RESULT_FAILED;
		}

		if( target.m_uid == USER_STORE_ROOT_UID ){
			if( nullptr != m_terminal ) m_terminal->writeln_ro(RODT_ATTR("\ncannot delete root"));
			return CMD_RESULT_FAILED;
		}

		if( __user_store_service.removeUser(uname) ){
			if( nullptr != m_terminal ){
				m_terminal->write_ro(RODT_ATTR("\nremoved "));
				m_terminal->write(uname);
			}
			return CMD_RESULT_OK;
		}

		return CMD_RESULT_FAILED;
	}
};

#endif

#endif
